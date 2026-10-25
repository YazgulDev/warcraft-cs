#include "GameBuild.hpp"
#include "WeaponTargetFilter.hpp"
#include <cmath>

namespace {
template<class T> bool Read(uintptr_t address,T& value) {
    SIZE_T got=0;return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&value,sizeof(value),&got) && got==sizeof(value);
}
}
bool WeaponTargetFilter::PassThrough(wc3::Handle unit,uintptr_t gameBase) {
    // This adapter requires engine internals that are not yet verified on 1.27a.
    if(wc3::ActiveBuild()==wc3::GameBuildKind::Classic127a) return false;

    // Locust removes targeting/collision; a web's large visual bounds must not absorb bullets.
    if(wc3::GetUnitAbilityLevel(unit,0x416C6F63)>0)return true;
    // An invulnerable hero or structure still has a physical body; never infer transparency from immunity alone.
    if(!gameBase || wc3::IsUnitType(unit,0) || wc3::IsUnitType(unit,2))return false;
    uintptr_t object=wc3::UnitObject(unit);unsigned flags=0,index=0,generation=0;
    if(!object || !Read(object+0x20,flags) || !(flags&8) ||
        !Read(object+0x16C,index) || !Read(object+0x170,generation))return false;
    // 1.26a's impact distance helper resolves this position reference and reads its live collision radius.
    // Read actual collision, not model size or movement speed; unknown geometry remains an obstacle.
    using Position=uintptr_t (__fastcall*)(unsigned,unsigned);
    uintptr_t position=reinterpret_cast<Position>(gameBase + wc3::Rva(0x3FA30))(index,generation);float radius=-1;
    if(!Read(position ? position+0x90 : 0,radius) || !std::isfinite(radius) || radius<0)return false;
    if(radius==0)return true;
    // SetUnitPathing(false) can keep the authored radius while clearing its live collision mask.
    // The native position-mask getter (+476340) reads this profile's low 24 collision bits.
    uintptr_t profile=0;unsigned mask=0;
    return Read(position+0xA8,profile) && profile && Read(profile+0x9C,mask) && !(mask&0xFFFFFF);
}
