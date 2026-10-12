#include "UnitCollisionProfile.hpp"
#include <cmath>

namespace {
template<class T> bool ReadField(uintptr_t address,T& value) {
    SIZE_T got=0;return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&value,sizeof(value),&got) && got==sizeof(value);
}
}
UnitCollisionProfile UnitCollisionProfile::Read(wc3::Handle unit,uintptr_t gameBase) {
    if(!unit || !gameBase)return {};
    // Locust helpers have visual bodies but deliberately do not participate in native movement collision.
    if(wc3::GetUnitAbilityLevel(unit,0x416C6F63)>0)return {true,false,0};
    uintptr_t object=wc3::UnitObject(unit);unsigned index=0,generation=0;
    if(!object || !ReadField(object+0x16C,index) || !ReadField(object+0x170,generation))return {};
    // The verified native distance helper resolves this live position reference; scale does not resize its collision.
    using Position=uintptr_t (__fastcall*)(unsigned,unsigned);
    uintptr_t position=reinterpret_cast<Position>(gameBase+0x3FA30)(index,generation),profile=0;
    float radius=0;unsigned mask=0;
    if(!ReadField(position ? position+0x90 : 0,radius) || !std::isfinite(radius) || radius<0 || radius>64 ||
        !ReadField(position+0xA8,profile) || !profile || !ReadField(profile+0x9C,mask))return {};
    // SetUnitPathing(false) clears the live collision mask even when its original radius remains nonzero.
    // Native getter 476670 adds five to the float exponent: position radii are in 32-unit cells, not world units.
    return {true,radius>0 && (mask&0xFFFFFF)!=0,radius*32};
}
