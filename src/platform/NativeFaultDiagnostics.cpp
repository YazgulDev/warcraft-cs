#include "NativeFaultDiagnostics.hpp"
#include "DiagnosticLog.hpp"
#include <cstdint>
#include <cstdarg>

namespace {
void Error(const char* format,...) {
    va_list args;va_start(args,format);DiagnosticLog::Write("ERROR",format,args);va_end(args);
}
void Address(const char* phase,const char* kind,unsigned index,uintptr_t address) {
    MEMORY_BASIC_INFORMATION region={};char module[MAX_PATH]={};uintptr_t base=0;
    if(VirtualQuery(reinterpret_cast<void*>(address),&region,sizeof(region)) && region.Type==MEM_IMAGE) {
        base=reinterpret_cast<uintptr_t>(region.AllocationBase);
        GetModuleFileNameA(reinterpret_cast<HMODULE>(base),module,MAX_PATH);
    }
    Error("native fault trace phase=%s kind=%s index=%u address=%p module=%s base=%p offset=%08X",
        phase,kind,index,reinterpret_cast<void*>(address),module[0] ? module : "unknown",reinterpret_cast<void*>(base),unsigned(base ? address-base : 0));
}
}
LONG NativeFaultDiagnostics::Filter(const char* phase,EXCEPTION_POINTERS* exception) {
    if(!exception || !exception->ExceptionRecord || !exception->ContextRecord)return EXCEPTION_EXECUTE_HANDLER;
    const auto& record=*exception->ExceptionRecord;const auto& context=*exception->ContextRecord;
    Error("native fault phase=%s code=%08lX flags=%08lX parameters=%lu",phase,record.ExceptionCode,record.ExceptionFlags,record.NumberParameters);
    Address(phase,"exception",0,reinterpret_cast<uintptr_t>(record.ExceptionAddress));
    // Raw exception data retains access type/address and custom exception arguments without dereferencing them.
    for(unsigned i=0;i<record.NumberParameters && i<EXCEPTION_MAXIMUM_PARAMETERS;++i)
        Error("native fault parameter phase=%s index=%u value=%p",phase,i,reinterpret_cast<void*>(record.ExceptionInformation[i]));
#ifdef _M_IX86
    Error("native fault registers phase=%s eip=%08lX esp=%08lX ebp=%08lX eax=%08lX ebx=%08lX ecx=%08lX edx=%08lX esi=%08lX edi=%08lX eflags=%08lX",
        phase,context.Eip,context.Esp,context.Ebp,context.Eax,context.Ebx,context.Ecx,context.Edx,context.Esi,context.Edi,context.EFlags);
    // Optimized/native code can omit frame pointers: this bounded chain is best effort, never a claimed full stack.
    uintptr_t frame=context.Ebp;unsigned frames=0;
    while(frames<24 && frame>=context.Esp && frame-context.Esp<1024*1024) {
        uintptr_t pair[2]={};SIZE_T read=0;
        if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(frame),pair,sizeof(pair),&read) || read!=sizeof(pair) || !pair[1])break;
        Address(phase,"frame-chain",frames++,pair[1]);
        if(pair[0]<=frame)break;frame=pair[0];
    }
    Error("native fault trace end phase=%s frames=%u limit=24 unwind=frame-pointer-best-effort",phase,frames);
#endif
    return EXCEPTION_EXECUTE_HANDLER;
}
