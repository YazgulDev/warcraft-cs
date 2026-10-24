#include "GameBuild.hpp"
#include "NativeFrameRate.hpp"
#include "WarcraftApi.hpp"
#include <algorithm>
#include <array>
#include <cstring>

namespace {
// Owned 1.26.0.6401 disassembly: render completion clears demand bit 4 at [esi+44].
// Keeping that bit permits more frames without changing game clocks or simulation timers.
constexpr uintptr_t DemandInstruction=0x62D7F2;
constexpr unsigned char signature[]={0x8B,0x46,0x44,0xA8,0x04,0x74,0x14,0x83,0xE0,0xFB};
bool Matches(const unsigned char* bytes,bool changed) {
    // Preserve exact opcodes while selecting the register/branch encoding of the accepted host.
    auto expected=std::array<unsigned char,sizeof(signature)>{};std::copy(signature,signature+sizeof(signature),expected.begin());
    if(wc3::ActiveBuild()==wc3::GameBuildKind::Classic127a){expected[1]=0x47;expected[6]=0x11;}
    return !std::memcmp(bytes,expected.data(),sizeof(signature)-1) &&
        (bytes[9]==expected[9] || (changed && bytes[9]==0xFF));
}

}
NativeFrameRate::~NativeFrameRate() { if (timer_) CloseHandle(timer_); }
bool NativeFrameRate::Install(uintptr_t gameBase) {
    unsigned char bytes[sizeof(signature)];SIZE_T got=0;
    auto address=reinterpret_cast<unsigned char*>(gameBase+wc3::Rva(DemandInstruction));
    if (!ReadProcessMemory(GetCurrentProcess(),address,bytes,sizeof(bytes),&got) || got!=sizeof(bytes) ||
        !Matches(bytes,false)) {
        wc3::Log("Frame-rate signature mismatch; native limiter retained");return false;
    }
    instruction_=address;
    QueryPerformanceFrequency(&frequency_);
    // Windows 10/11 high-resolution waits avoid imposing a global timer-resolution change.
    timer_=CreateWaitableTimerExW(nullptr,nullptr,0x2,TIMER_ALL_ACCESS);
    bool highResolution=timer_!=nullptr;
    if (!timer_) timer_=CreateWaitableTimerW(nullptr,FALSE,nullptr);
    wc3::Log("Frame-rate adapter ready highResolutionTimer=%d",highResolution);
    return true;
}
bool NativeFrameRate::WriteDemand(bool unlocked) {
    unsigned char bytes[sizeof(signature)];SIZE_T got=0;
    if (!ReadProcessMemory(GetCurrentProcess(),instruction_,bytes,sizeof(bytes),&got) || got!=sizeof(bytes) ||
        !Matches(bytes,true)) {
        wc3::LogError("Frame-rate instruction changed; refusing overwrite");return false;
    }
    unsigned char value=unlocked ? 0xFF : 0xFB;
    if (bytes[9]==value) return true;
    DWORD protection=0;
    if (!VirtualProtect(instruction_+9,1,PAGE_EXECUTE_READWRITE,&protection)) {
        wc3::LogError("Frame-rate protection failed win32=%lu",GetLastError());return false;
    }
    // A single operand byte changes only render demand; no on-disk game file is patched.
    instruction_[9]=value;
    FlushInstructionCache(GetCurrentProcess(),instruction_+9,1);
    DWORD ignored=0;
    if (!VirtualProtect(instruction_+9,1,protection,&ignored))
        wc3::LogError("Frame-rate protection restore failed win32=%lu",GetLastError());
    return true;
}
void NativeFrameRate::Configure(int maximumFPS) {
    if (requested_==maximumFPS) return;
    requested_=maximumFPS;previous_=0;
    int target=maximumFPS==0 ? 0 : std::clamp(maximumFPS,30,360);
    maximum_=instruction_ && WriteDemand(target>0) ? target : 0;
    wc3::Log("Frame-rate applied requested=%d cap=%d nativeLimiter=%d",maximumFPS,maximum_,maximum_==0);
}
void NativeFrameRate::Pace() {
    if (!maximum_ || frequency_.QuadPart<=0) return;
    LARGE_INTEGER now;QueryPerformanceCounter(&now);
    LONGLONG period=frequency_.QuadPart/maximum_;
    LONGLONG deadline=previous_+period;
    // Focus pauses, slow frames and live reloads never accumulate a catch-up burst of frames.
    if (!previous_ || now.QuadPart>=deadline) {previous_=now.QuadPart;return;}
    while (now.QuadPart<deadline) {
        LONGLONG remaining=deadline-now.QuadPart;
        LARGE_INTEGER due;due.QuadPart=-std::max<LONGLONG>(1,remaining*10000000/frequency_.QuadPart);
        if (timer_ && SetWaitableTimer(timer_,&due,0,nullptr,nullptr,FALSE)) WaitForSingleObject(timer_,50);
        else Sleep(1);
        QueryPerformanceCounter(&now);
    }
    previous_=now.QuadPart;
}
