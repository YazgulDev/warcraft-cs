#include "NativeExplosiveDeath.hpp"
#include "NativeRegistry.hpp"
#include "ExplodedFlagLayout.hpp"
#include "GameBuild.hpp"

namespace {
using Setter=void (__cdecl*)(wc3::Handle,BOOL);
Setter setter=nullptr;ExplodedFlagLayout layout;unsigned generation=0;
bool Flags(uintptr_t object,uint32_t& flags) {
    SIZE_T got=0;return object && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(object+layout.offset),&flags,4,&got) && got==4;
}
}
void NativeExplosiveDeath::Configure(uintptr_t module) {
    ++generation;setter=nullptr;layout={};
    // Reading the previous CUnit flag is verified only on the owned 1.26a host. Public portal
    // queries may resolve independently on other builds; an unverified corpse layout stays untouched.
    if(wc3::ActiveBuild()!=wc3::GameBuildKind::Classic126a){wc3::Log("Explosive death adapter unavailable on this host");return;}
    auto function=NativeRegistry::Resolve(module,"SetUnitExploded","(Hunit;B)V");
    unsigned char bytes[96]={};SIZE_T got=0;
    if(function && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(function),bytes,sizeof(bytes),&got) && got==sizeof(bytes))
        layout=ExplodedFlagLayout::Decode(bytes,sizeof(bytes),function,module+wc3::Rva(0x3BDCB0));
    if(layout.mask)setter=reinterpret_cast<Setter>(function);
    wc3::Log("Explosive death adapter verified=%d offset=%u mask=%08X",setter!=nullptr,layout.offset,layout.mask);
}
void NativeExplosiveDeath::Reset(){++generation;}
bool NativeExplosiveDeath::Available(){return setter!=nullptr;}
#ifdef WCS_NATIVE_INTERACTIONS_TEST
void NativeExplosiveDeath::TestBinding(void (__cdecl* function)(wc3::Handle,BOOL),unsigned offset,uint32_t mask) {
    ++generation;setter=function;layout={offset,mask};
}
#endif
NativeExplosiveDeath::Lease::Lease(wc3::Handle actor,wc3::Handle victim,bool enabled) {
    // Only hostile living creatures qualify. Native map protection and every damage calculation remain intact.
    if(!enabled || !setter || !actor || !victim || !wc3::GetUnitTypeId(victim) || wc3::IsUnitType(victim,2) ||
        !wc3::IsUnitEnemy(victim,wc3::GetOwningPlayer(actor)) || wc3::Real(wc3::GetUnitState(victim,0))<=.405f)return;
    victim_=victim;type_=wc3::GetUnitTypeId(victim);object_=wc3::UnitObject(victim);generation_=generation;
    uint32_t before=0;if(!Flags(object_,before) || (before&layout.mask))return; // Keep a map-owned exploded flag untouched.
    setter(victim,TRUE);uint32_t after=0;
    restore_=generation==generation_ && wc3::UnitObject(victim)==object_ && Flags(object_,after) && (after&layout.mask);
}
NativeExplosiveDeath::Lease::~Lease(){Release();}
void NativeExplosiveDeath::Lease::Release() {
    // Survivors regain their original state. A native exploded corpse keeps its native death lifecycle;
    // removal, resurrection/morph identity changes and map replacement cannot restore into a stale object.
    bool owned=restore_;restore_=false;
    if(!owned || generation!=generation_ || wc3::UnitObject(victim_)!=object_ || wc3::GetUnitTypeId(victim_)!=type_ ||
        wc3::Real(wc3::GetUnitState(victim_,0))<=.405f)return;
    uint32_t flags=0;if(Flags(object_,flags) && (flags&layout.mask))setter(victim_,FALSE);
}
