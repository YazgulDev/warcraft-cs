#pragma once
#include "WarcraftApi.hpp"

// Gates/crates are widgets distinct from units. Keep their native identity and life access together.
struct NativeAttackTarget {
    wc3::Handle handle=0;
    uintptr_t object=0;
    bool destructable=false;
    static NativeAttackTarget Resolve(wc3::Handle handle) {
        uintptr_t object=wc3::UnitObject(handle);
        return object ? NativeAttackTarget{handle,object,false} : NativeAttackTarget{handle,wc3::DestructableObject(handle),true};
    }
    bool Valid() const {
        return handle && object && object==(destructable ? wc3::DestructableObject(handle) : wc3::UnitObject(handle)) &&
            (destructable ? wc3::GetDestructableTypeId(handle) : wc3::GetUnitTypeId(handle))!=0;
    }
    float Life() const {
        return Valid() ? wc3::Real(destructable ? wc3::GetDestructableLife(handle) : wc3::GetUnitState(handle,0)) : 0.f;
    }
    bool Eligible(wc3::Handle player,bool targetAllies) const {
        if(Life()<=.405f)return false;
        // Native attack validation decides immunity/weapon eligibility; the mod never changes quest protection.
        if(destructable)return true;
        return !wc3::IsUnitHidden(handle) && !wc3::IsUnitLoaded(handle) && wc3::IsUnitVisible(handle,player) &&
            (targetAllies || wc3::IsUnitEnemy(handle,player) || wc3::IsNeutralUnit(handle));
    }
};
