#include "GameBuild.hpp"
#include "NativeMovementGuard.hpp"
#include <cstring>
#include <intrin.h>

namespace {
uintptr_t gameBase=0;
wc3::Handle (*viewActor)()=nullptr;
bool (*nativeCast)()=nullptr;
using Speed=float* (__thiscall*)(uintptr_t,float*);
Speed originalSpeed=nullptr;
float* __fastcall WalkingSpeed(uintptr_t movement,uintptr_t,float* result) {
    uintptr_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    // CUnit::GetMoveSpeed is the stats/JASS path. Keep slows, roots and map speed queries native.
    if(caller==gameBase + wc3::Rva(0x26DE44) || (nativeCast && nativeCast()))return originalSpeed(movement,result);
    auto unit=viewActor ? viewActor() : 0;uintptr_t actor=wc3::UnitObject(unit);
    if(actor && *reinterpret_cast<uintptr_t*>(actor+0x1EC)==movement) {
        *result=0;static DWORD last=0;DWORD now=GetTickCount();
        if(now-last>=1000) {last=now;wc3::Trace("FPS blocked native walking unit=%08X caller=%08X",unit,caller-gameBase);}
        return result;
    }
    return originalSpeed(movement,result);
}
}
void NativeMovementGuard::Install(uintptr_t base,wc3::Handle (*actor)(),bool (*casting)(),FpsCombatGuard::HookInstaller hook) {
    // This adapter requires engine internals that are not yet verified on 1.27a.
    if(wc3::ActiveBuild()==wc3::GameBuildKind::Classic127a) return;

    gameBase=base;viewActor=actor;nativeCast=casting;
    // This motor getter tests Amov's disable counter; no persistent unit data is written by the hook.
    const unsigned char motor[]={0x51,0x83,0x79,0x7C,0x00};
    const unsigned char stats[]={0xE8};
    int relative=0;std::memcpy(&relative,reinterpret_cast<void*>(base + wc3::Rva(0x26DE40)),4);
    bool verified=!std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x201C60)),motor,sizeof(motor)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x26DE3F)),stats,sizeof(stats)) && 0x26DE44+relative==0x201C60;
    bool installed=verified && hook(reinterpret_cast<void*>(base + wc3::Rva(0x201C60)),reinterpret_cast<void*>(WalkingSpeed),reinterpret_cast<void**>(&originalSpeed));
    wc3::Log("FPS native walking guard verified=%d installed=%d",verified,installed);
}
