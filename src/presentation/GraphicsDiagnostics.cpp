#include "GraphicsDiagnostics.hpp"
#include "HudDiagnostics.hpp"
#include "../platform/DiagnosticLog.hpp"
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>

namespace {
void Write(const char* level,const char* format,...) {va_list args;va_start(args,format);DiagnosticLog::Write(level,format,args);va_end(args);}
template<class T>T Proc(const char* name) {
    auto value=wglGetProcAddress(name);auto address=reinterpret_cast<uintptr_t>(value);
    return address>3 && address!=UINTPTR_MAX ? reinterpret_cast<T>(value) : nullptr;
}
const char* ErrorName(GLenum error) {
    switch(error){case GL_INVALID_ENUM:return "INVALID_ENUM";case GL_INVALID_VALUE:return "INVALID_VALUE";
    case GL_INVALID_OPERATION:return "INVALID_OPERATION";case GL_STACK_OVERFLOW:return "STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:return "STACK_UNDERFLOW";case GL_OUT_OF_MEMORY:return "OUT_OF_MEMORY";
    case 0x506:return "INVALID_FRAMEBUFFER_OPERATION";case 0x507:return "CONTEXT_LOST";default:return "UNKNOWN";}
}
// Bounded deduplication retains the first/new failure immediately and reports repeats on its next occurrence.
struct Entry {unsigned long long key=0;DWORD tick=0;unsigned repeats=0;};
std::array<Entry,64> entries;unsigned cursor=0;SRWLOCK gate=SRWLOCK_INIT;
unsigned long long Hash(const char* text,unsigned long long seed=1469598103934665603ULL) {
    for(;text && *text;++text){seed^=static_cast<unsigned char>(*text);seed*=1099511628211ULL;}return seed ? seed : 1;
}
bool Allow(unsigned long long key,unsigned& repeats) {
    auto settings=DiagnosticLog::Settings();DWORD now=GetTickCount();repeats=0;
    AcquireSRWLockExclusive(&gate);
    for(auto& entry:entries)if(entry.key==key) {
        if(now-entry.tick<settings.intervalMs){++entry.repeats;ReleaseSRWLockExclusive(&gate);return false;}
        repeats=entry.repeats;entry.repeats=0;entry.tick=now;ReleaseSRWLockExclusive(&gate);return true;
    }
    entries[cursor++%entries.size()]={key,now,0};ReleaseSRWLockExclusive(&gate);return true;
}
void Enable(GLenum cap,bool enabled){if(enabled)glEnable(cap);else glDisable(cap);}
}
GraphicsDiagnostics::GraphicsDiagnostics(const LoggingSettings& settings) {
    enabled_=settings.graphicsDebug && !GraphicsBackend::IsD3D9() && wglGetCurrentContext();if(!enabled_)return;
    context_=wglGetCurrentContext();static thread_local DWORD sampled=0;trace_=DiagnosticLog::Due(sampled);
    Drain(); // Label queued native/world errors before any mod resource upload or state preparation.
    auto ext=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));int major=1,minor=1;
    if(auto version=reinterpret_cast<const char*>(glGetString(GL_VERSION)))sscanf_s(version,"%d.%d",&major,&minor);
    khr_=major>4 || (major==4 && minor>=3) || (ext && std::strstr(ext,"GL_KHR_debug"));
    install_=khr_ ? Proc<Install>("glDebugMessageCallback") : ext && std::strstr(ext,"GL_ARB_debug_output") ? Proc<Install>("glDebugMessageCallbackARB") : nullptr;
    if(install_) {
        void* callback=nullptr;void* user=nullptr;glGetPointerv(0x8244,&callback);glGetPointerv(0x8245,&user);
        previous_=reinterpret_cast<Callback>(callback);previousUser_=user;
        output_=khr_ && glIsEnabled(0x92E0);synchronous_=glIsEnabled(0x8242)!=0;
        // Synchronous delivery keeps phase labels and the local callback owner valid; filters remain untouched.
        glEnable(0x8242);if(khr_)glEnable(0x92E0);install_(Driver,this);
    }
    if(trace_)Write("TRACE","graphics observer context=%p driver=%s synchronous=%d filters=preserved",context_,install_ ? khr_ ? "KHR_debug" : "ARB_debug_output" : "error-checkpoints",install_!=nullptr);
}
GraphicsDiagnostics::~GraphicsDiagnostics() {
    if(!enabled_)return;Drain();
    if(install_) {install_(previous_,previousUser_);Enable(0x8242,synchronous_);if(khr_)Enable(0x92E0,output_);}
}
void GraphicsDiagnostics::Phase(const char* next) {
    if(!enabled_)return;Drain();phase_=next;
    if(trace_)Write("TRACE","graphics phase=%s context=%p thread=%lu",phase_,context_,GetCurrentThreadId());
}
void GraphicsDiagnostics::Drain() {
    bool failed=driverError_;driverError_=false;
    // Poll every boundary, not only the once-per-second reticle sample. A broken driver cannot loop forever.
    for(unsigned i=0;i<16;++i) {
        GLenum error=glGetError();if(error==GL_NO_ERROR)break;failed=true;unsigned repeats=0;
        auto key=Hash(phase_,Hash(ErrorName(error)))^error^reinterpret_cast<uintptr_t>(context_);
        if(Allow(key,repeats))Write("ERROR","graphics OpenGL phase=%s context=%p code=%04X name=%s repeats=%u origin=%s",phase_,context_,error,ErrorName(error),repeats,
            !std::strcmp(phase_,"host-before-overlay") ? "host-or-earlier-pass" : "since-previous-boundary");
        if(i==15)Write("ERROR","graphics error queue limit reached phase=%s context=%p",phase_,context_);
    }
    if(failed) {
        // A failure snapshot is collected outside callbacks/glBegin; Detailed=false still retains the ERROR record.
        unsigned repeats=0;if(Allow(Hash(phase_,0xFA17)^reinterpret_cast<uintptr_t>(context_),repeats))HudDiagnostics::State(phase_,true);
    }
}
void APIENTRY GraphicsDiagnostics::Driver(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const char* message,const void* user) {
    auto self=static_cast<GraphicsDiagnostics*>(const_cast<void*>(user));if(!self)return;
    static thread_local bool writing=false;if(writing)return;writing=true;
    char bounded[2049]={};size_t n=message ? length>=0 ? std::min(size_t(length),sizeof(bounded)-1) : strnlen_s(message,sizeof(bounded)-1) : 0;
    if(n)std::memcpy(bounded,message,n);for(size_t i=0;i<n;++i)if(bounded[i]=='\n' || bounded[i]=='\r')bounded[i]=' ';
    bool error=type==0x824C || severity==0x9146;self->driverError_|=error;
    const char* level=error ? "ERROR" : severity==0x9147 || type==0x824E ? "WARN" : "TRACE";unsigned repeats=0;
    auto key=Hash(bounded,Hash(self->phase_))^id^(static_cast<unsigned long long>(source)<<32)^type^severity^reinterpret_cast<uintptr_t>(self->context_);
    if(Allow(key,repeats))Write(level,"graphics driver phase=%s context=%p source=%04X type=%04X id=%u severity=%04X repeats=%u message=%s",self->phase_,self->context_,source,type,id,severity,repeats,bounded);
    // Keep an existing enabled observer working; never call GL or consume its error queue from a callback.
    if(self->previous_ && self->previous_!=Driver && (!self->khr_ || self->output_))self->previous_(source,type,id,severity,length,message,self->previousUser_);
    writing=false;
}
bool GraphicsDiagnostics::Result(const char* operation,long result,unsigned parameter,unsigned value) {
    if(result>=0)return true;if(!DiagnosticLog::Settings().graphicsDebug)return false;unsigned repeats=0;
    auto key=Hash(operation)^static_cast<unsigned long>(result)^(static_cast<unsigned long long>(parameter)<<32)^value;
    if(Allow(key,repeats))Write("ERROR","graphics D3D9 operation=%s hr=%08lX parameter=%u value=%u repeats=%u",operation,static_cast<unsigned long>(result),parameter,value,repeats);
    return false;
}
