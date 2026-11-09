#include "GameBuild.hpp"
#include "NativeMovementGuard.hpp"
#include <cstring>
#include <intrin.h>

namespace {
uintptr_t gameBase=0;
wc3::Handle (*viewActor)()=nullptr;
bool (*nativeCast)()=nullptr;
wc3::Handle (*airborneActor)()=nullptr;
using RepairPosition=int (__thiscall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
RepairPosition originalRepair=nullptr;
constexpr uintptr_t RepairRva=0x4944A0, RefreshCallRva=0x49572C;
bool Calls(uintptr_t from,uintptr_t target) {
    if(*reinterpret_cast<const unsigned char*>(from)!=0xE8)return false;
    int32_t relative=0;
    std::memcpy(&relative,reinterpret_cast<const void*>(from+1),sizeof(relative));
    return from+5+relative==target;
}
int __fastcall RepairGroundPosition(uintptr_t position,uintptr_t,uintptr_t mask,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d) {
    // The path-mask refresh can relocate a ground unit before its walking speed is read.
    // Only that internal call treats the already swept airborne position as valid.
    auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    if(caller==gameBase+RefreshCallRva+5 && airborneActor) {
        auto unit=airborneActor();
        auto object=unit ? wc3::UnitObject(unit) : 0;
        if(object) {
            using Position=uintptr_t (__fastcall*)(unsigned,unsigned);
            auto resolve=reinterpret_cast<Position>(gameBase+0x3FA30);
            // These are the live position reference fields also used by UnitCollisionProfile.
            auto ownPosition=resolve(*reinterpret_cast<const unsigned*>(object+0x16C),
                *reinterpret_cast<const unsigned*>(object+0x170));
            if(position && position==ownPosition) {
                static DWORD last=0;
                DWORD now=GetTickCount();
                if(now-last>=1000) {
                    last=now;
                    wc3::Trace("FPS skipped native ground repair during flight unit=%08X",unit);
                }
                return 0; // Native result for a position which needs no repair.
            }
        }
    }
    return originalRepair(position,mask,a,b,c,d);
}
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
void NativeMovementGuard::Install(uintptr_t base,wc3::Handle (*actor)(),bool (*casting)(),FpsCombatGuard::HookInstaller hook,
    wc3::Handle (*flightActor)()) {
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
    if(!flightActor)return;
    airborneActor=flightActor;
    // Verify the SEH prologue, five stack arguments, and the path-mask caller before hooking.
    const unsigned char repair[]={0x64,0xA1,0,0,0,0,0x50,0x83,0xEC,0x24,0x53,0x55,0x56,0x57};
    const unsigned char repairReturn[]={0xC2,0x14,0};
    const unsigned char refresh[]={0x6A,0x02,0x52,0x8B,0x54,0x24,0x2C,0x52,0x51,0x50,0x8B,0xCE};
    auto repairEntry=base+RepairRva;
    bool repairVerified=!std::memcmp(reinterpret_cast<void*>(repairEntry+7),repair,sizeof(repair)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x4945EC),repairReturn,sizeof(repairReturn)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x495720),refresh,sizeof(refresh)) &&
        Calls(base+RefreshCallRva,repairEntry);
    bool repairInstalled=repairVerified && hook(reinterpret_cast<void*>(repairEntry),reinterpret_cast<void*>(RepairGroundPosition),
        reinterpret_cast<void**>(&originalRepair));
    wc3::Log("FPS airborne ground repair guard verified=%d installed=%d",repairVerified,repairInstalled);
}
