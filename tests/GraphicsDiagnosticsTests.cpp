#include "../src/presentation/GraphicsDiagnostics.hpp"
#include "../src/presentation/HudDiagnostics.hpp"
#include "../src/platform/DiagnosticLog.hpp"
#include "../src/platform/NativeFaultDiagnostics.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

namespace {
using Callback=void (APIENTRY*)(GLenum,GLenum,GLuint,GLenum,GLsizei,const char*,const void*);
using Install=void (APIENTRY*)(Callback,const void*);
using Insert=void (APIENTRY*)(GLenum,GLenum,GLuint,GLenum,GLsizei,const char*);
using Control=void (APIENTRY*)(GLenum,GLenum,GLenum,GLsizei,const GLuint*,GLboolean);
unsigned forwarded=0;
int cookie=0;
bool RaiseAndRecover() {
    // The real SEH filter must preserve recovery and log the original context even with Detailed=false.
    __try {ULONG_PTR data[]={0x1234,0x5678};RaiseException(0xE0424343,0,2,data);}
    __except(NativeFaultDiagnostics::Filter("test-native-fault",GetExceptionInformation())) {return true;}
    return false;
}
void APIENTRY Previous(GLenum,GLenum,GLuint id,GLenum,GLsizei,const char*,const void* user) {
    assert(user==&cookie);if(id==49002 || id==49003)++forwarded;
}
template<class T>T Proc(const char* name) {auto p=reinterpret_cast<T>(wglGetProcAddress(name));assert(p);return p;}
std::string Read(const char* directory) {
    std::ifstream file(std::string(directory)+"/WarcraftCS.log",std::ios::binary);
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
size_t Count(const std::string& text,const char* needle) {
    size_t count=0,start=0;while((start=text.find(needle,start))!=std::string::npos){++count;start+=strlen(needle);}return count;
}
}
int main(int argc,char** argv) {
    _set_error_mode(_OUT_TO_STDERR);assert(argc==2);
    LoggingSettings settings;settings.intervalMs=60000;
    DiagnosticLog::Configure(settings);DiagnosticLog::Open(argv[1]);
    HWND window=CreateWindowA("STATIC","Graphics observer regression",WS_POPUP,0,0,320,240,nullptr,nullptr,nullptr,nullptr);assert(window);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR pixel={};pixel.nSize=sizeof(pixel);pixel.nVersion=1;
    pixel.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pixel.iPixelType=PFD_TYPE_RGBA;pixel.cColorBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&pixel),&pixel));
    for(unsigned cycle=0;cycle<3;++cycle) {
        HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));
        auto install=Proc<Install>("glDebugMessageCallback");auto insert=Proc<Insert>("glDebugMessageInsert");
        auto control=Proc<Control>("glDebugMessageControl");
        // Only this disposable context opens all message filters; the production observer preserves them.
        control(GL_DONT_CARE,GL_DONT_CARE,GL_DONT_CARE,0,nullptr,GL_TRUE);
        GLuint filtered=49004;control(0x824A,0x8251,GL_DONT_CARE,1,&filtered,GL_FALSE);
        install(Previous,&cookie);glDisable(0x92E0);glDisable(0x8242);
        glEnable(0xDEAD); // Pending host error must be labelled before mod preparation begins.
        {
            GraphicsDiagnostics observer(settings);assert(glGetError()==GL_NO_ERROR);
            assert(glIsEnabled(0x92E0) && glIsEnabled(0x8242));
            observer.Phase("test-held-model");
            glEnable(0xDEAE);observer.Phase("test-flat-hud");
            assert(glGetError()==GL_NO_ERROR);
            insert(0x824A,0x824C,49001,0x9146,-1,"test driver high\nsecond line");
            insert(0x824A,0x8251,filtered,0x9146,-1,"filtered-must-not-appear");
        }
        assert(!glIsEnabled(0x92E0) && !glIsEnabled(0x8242));
        void* callback=nullptr;void* user=nullptr;glGetPointerv(0x8244,&callback);glGetPointerv(0x8245,&user);
        assert(callback==reinterpret_cast<void*>(Previous) && user==&cookie && forwarded==cycle*2);
        glEnable(0x92E0);glEnable(0x8242);
        {
            GraphicsDiagnostics observer(settings);observer.Phase("test-chain");
            insert(0x824A,0x8251,49002,0x9147,-1,"test driver medium");
        }
        assert(glIsEnabled(0x92E0) && glIsEnabled(0x8242));
        insert(0x824A,0x8251,49003,0x9148,-1,"restored foreign observer");
        assert(forwarded==(cycle+1)*2 && glGetError()==GL_NO_ERROR);
        install(nullptr,nullptr);glDisable(0x92E0);glDisable(0x8242);
        // Turning off the observer must neither install callbacks nor consume the caller's error queue.
        settings.graphicsDebug=false;
        {GraphicsDiagnostics observer(settings);observer.Phase("disabled-must-not-appear");glEnable(0xDEAD);}
        assert(glGetError()==GL_INVALID_ENUM && !glIsEnabled(0x92E0));settings.graphicsDebug=true;
        wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    // A new accepted round/visibility transition is useful even inside the ordinary sampling interval.
    HudDiagnostics::Weapon(1,100,"shoot",.01f,true,true,false,false,false);
    HudDiagnostics::Weapon(1,100,"shoot",.02f,true,true,false,false,false);
    HudDiagnostics::Weapon(1,101,"shoot",.01f,true,true,false,false,false);
    HudDiagnostics::Weapon(1,101,"shoot",.02f,true,false,false,false,true);
    // HRESULT errors survive Detailed=false; success/opt-out and duplicate suppression have separate contracts.
    settings.detailed=false;DiagnosticLog::Configure(settings);
    const long invalid=static_cast<long>(0x8876086CUL);
    assert(GraphicsDiagnostics::Result("test-success",0));
    assert(!GraphicsDiagnostics::Result("test-failure",invalid,7,12));
    assert(!GraphicsDiagnostics::Result("test-failure",invalid,7,12));
    assert(Count(Read(argv[1]),"operation=test-failure ")==1);
    settings.intervalMs=1;DiagnosticLog::Configure(settings);Sleep(32); // GetTickCount may advance only every 16 ms.
    assert(!GraphicsDiagnostics::Result("test-failure",invalid,7,12));
    assert(!GraphicsDiagnostics::Result("test-other-parameter",invalid,8,12));
    settings.graphicsDebug=false;DiagnosticLog::Configure(settings);
    assert(!GraphicsDiagnostics::Result("disabled-result-must-not-appear",invalid));
    settings.graphicsDebug=true;DiagnosticLog::Configure(settings);
    // A real queued GL error is still captured when optional TRACE summaries are disabled.
    HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));
    {GraphicsDiagnostics observer(settings);observer.Phase("test-errors-without-trace");glEnable(0xDEAD);}
    assert(glGetError()==GL_NO_ERROR);
    assert(RaiseAndRecover());
    DiagnosticLog::Close();auto log=Read(argv[1]);
    assert(log.find("graphics OpenGL phase=host-before-overlay")!=std::string::npos);
    assert(log.find("graphics OpenGL phase=test-held-model")!=std::string::npos);
    assert(log.find("phase=test-flat-hud context=")!=std::string::npos);
    assert(log.find("message=test driver high second line")!=std::string::npos);
    assert(log.find("[WARN]")!=std::string::npos && log.find("message=test driver medium")!=std::string::npos);
    assert(log.find("hr=8876086C parameter=7 value=12 repeats=1")!=std::string::npos);
    assert(log.find("graphics OpenGL phase=test-errors-without-trace")!=std::string::npos);
    assert(log.find("graphics phase=test-errors-without-trace")==std::string::npos);
    assert(log.find("disabled-must-not-appear")==std::string::npos && log.find("disabled-result-must-not-appear")==std::string::npos);
    assert(log.find("filtered-must-not-appear")==std::string::npos);
    assert(log.find("operation=test-success")==std::string::npos);
    assert(Count(log,"graphics weapon slot=1 ")==3 && log.find("reason=hidden")!=std::string::npos);
    assert(log.find("native fault phase=test-native-fault code=E0424343")!=std::string::npos);
    assert(log.find("kind=exception index=0")!=std::string::npos && log.find("module=unknown")==std::string::npos);
    assert(log.find("native fault registers phase=test-native-fault eip=")!=std::string::npos);
    assert(log.find("index=1 value=00005678")!=std::string::npos);
    wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
    puts("PASS real WGL errors/driver messages, callback restoration, contexts, HRESULTs, repeat counts, live opt-out and original native SEH context/recovery.");
}
