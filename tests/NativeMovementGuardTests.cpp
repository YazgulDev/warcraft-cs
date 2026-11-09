#include "../src/platform/NativeMovementGuard.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>

namespace {
uintptr_t base=0;wc3::Handle actor=1,flightActor=0;bool casting=false;
std::array<unsigned char,0x200> unit={};
using Speed=float* (__fastcall*)(uintptr_t,uintptr_t,float*);
Speed guarded=nullptr;int calls=0;
float* __fastcall Native(uintptr_t,uintptr_t,float* result) {++calls;*result=340;return result;}
using Repair=int (__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
Repair repairGuard=nullptr;
int repairs=0;
int __fastcall NativeRepair(uintptr_t,uintptr_t,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e) {
    assert(a==1 && b==2 && c==3 && d==4 && e==5);
    ++repairs;return 77;
}
bool Hook(void* address,void* callback,void** original) {
    if(reinterpret_cast<uintptr_t>(address)==base+0x4944A0) {
        repairGuard=reinterpret_cast<Repair>(callback);
        *original=reinterpret_cast<void*>(NativeRepair);
    } else {
        guarded=reinterpret_cast<Speed>(callback);
        *original=reinterpret_cast<void*>(Native);
    }
    return true;
}
}
namespace wc3 {
void Log(const char*,...) {}
void Trace(const char*,...) {}
uintptr_t UnitObject(Handle h) {return h==1 ? uintptr_t(unit.data()) : 0;}
}
int main() {
    static_assert(sizeof(uintptr_t)==4,"Native call-site verification is x86.");
    auto image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x500000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    assert(image);base=uintptr_t(image);uintptr_t movement=17;std::memcpy(unit.data()+0x1EC,&movement,4);
    auto actorGetter=[](){return actor;};auto cast=[](){return casting;};
    NativeMovementGuard::Install(base,actorGetter,cast,Hook);assert(!guarded);
    const unsigned char motor[]={0x51,0x83,0x79,0x7C,0};std::memcpy(image+0x201C60,motor,sizeof(motor));
    image[0x26DE3F]=0xE8;int delta=0x201C60-0x26DE44;std::memcpy(image+0x26DE40,&delta,4);
    NativeMovementGuard::Install(base,actorGetter,cast,Hook);assert(guarded);float result=-1;
    guarded(movement,0,&result);assert(!result && !calls);guarded(18,0,&result);assert(result==340 && calls==1);
    // Execute the real stats call-site ABI: its native result survives FPS even though walking returns zero.
    const unsigned char forward[]={0x90,0x90,0xFF,0x74,0x24,0x04};std::memcpy(image+0x26DE39,forward,sizeof(forward));
    delta=int(reinterpret_cast<uintptr_t>(guarded)-(base+0x26DE44));std::memcpy(image+0x26DE40,&delta,4);
    const unsigned char finish[]={0xC2,0x04,0};std::memcpy(image+0x26DE44,finish,sizeof(finish));
    reinterpret_cast<Speed>(base+0x26DE39)(movement,0,&result);assert(result==340 && calls==2);
    casting=true;guarded(movement,0,&result);assert(result==340 && calls==3);
    casting=false;actor=0;guarded(movement,0,&result);assert(result==340 && calls==4);
    // Recreate the verified call's stack and caller address, with a private position resolver.
    auto airborne=[](){return flightActor;};
    NativeMovementGuard::Install(base,actorGetter,cast,Hook,airborne);assert(!repairGuard);
    const unsigned char repair[]={0x64,0xA1,0,0,0,0,0x50,0x83,0xEC,0x24,0x53,0x55,0x56,0x57};
    const unsigned char repairReturn[]={0xC2,0x14,0};
    const unsigned char refresh[]={0x6A,0x02,0x52,0x8B,0x54,0x24,0x2C,0x52,0x51,0x50,0x8B,0xCE};
    std::memcpy(image+0x4944A7,repair,sizeof(repair));
    std::memcpy(image+0x4945EC,repairReturn,sizeof(repairReturn));
    std::memcpy(image+0x495720,refresh,sizeof(refresh));
    image[0x49572C]=0xE8;delta=0x4944A0-0x495731;std::memcpy(image+0x49572D,&delta,4);
    NativeMovementGuard::Install(base,actorGetter,cast,Hook,airborne);assert(repairGuard);
    const unsigned char resolve[]={0xB8,23,0,0,0,0xC3};std::memcpy(image+0x3FA30,resolve,sizeof(resolve));
    for(int i=0;i<5;++i) {
        const unsigned char pushArg[]={0xFF,0x74,0x24,0x14};
        std::memcpy(image+0x495718+4*i,pushArg,sizeof(pushArg));
    }
    delta=int(reinterpret_cast<uintptr_t>(repairGuard)-(base+0x495731));
    std::memcpy(image+0x49572D,&delta,4);
    std::memcpy(image+0x495731,repairReturn,sizeof(repairReturn));
    auto refreshCall=reinterpret_cast<Repair>(base+0x495718);
    flightActor=1;
    assert(refreshCall(23,0,1,2,3,4,5)==0 && repairs==0);
    assert(refreshCall(24,0,1,2,3,4,5)==77 && repairs==1);
    assert(repairGuard(23,0,1,2,3,4,5)==77 && repairs==2); // Other placement callers remain native.
    flightActor=0;
    assert(refreshCall(23,0,1,2,3,4,5)==77 && repairs==3);
    VirtualFree(image,0,MEM_RELEASE);std::puts("PASS native walking suppressed; stats, explicit spells, other units and RTS speed remain native.");
}
