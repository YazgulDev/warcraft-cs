#pragma once
#include "WarcraftApi.hpp"
#include "../combat/HeroAttackTimeline.hpp"
#include "../combat/UnitStatus.hpp"
#include "../config/GameplaySettings.hpp"
#include "../combat/UnitHitboxes.hpp"
#include "../combat/DestructableHitboxes.hpp"

// Direct unit attacks own their timeline and aimed contact; native ranged missiles never require a pursuit order.
class UnitAttack {
public:
    static bool Supported(wc3::Handle unit);
    void Reset() {timeline_.Reset();}
    void Cancel() {timeline_.Cancel();}
    bool Busy(double now) const {return timeline_.Busy(now);}
    bool ContactDue(double now) const {return timeline_.ContactDue(now);}
    bool Begin(wc3::Handle unit,double now,const GameplaySettings& settings,const UnitStatus& status);
    bool Tick(wc3::Handle unit,double now,float yaw,float feet,const float* aimOrigin,const float* aimDirection,bool casting,
        const GameplaySettings& settings,const UnitStatus& status,UnitHitboxes& units,DestructableHitboxes& destructables);
private:
    bool Contact(wc3::Handle unit,float yaw,float feet,const GameplaySettings& settings,
        UnitHitboxes& units,DestructableHitboxes& destructables);
    bool RangedContact(wc3::Handle unit,float feet,const float* aimOrigin,const float* aimDirection,
        const GameplaySettings& settings,UnitHitboxes& units,DestructableHitboxes& destructables);
    bool Damage(wc3::Handle actor,wc3::Handle target,bool destructable,float distance,bool ranged,const GameplaySettings& settings);
    HeroAttackTimeline timeline_;
};
