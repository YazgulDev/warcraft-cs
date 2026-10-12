#pragma once
#include "WarcraftApi.hpp"
#include "../combat/HeroAttackTimeline.hpp"
#include "../combat/UnitStatus.hpp"
#include "../config/GameplaySettings.hpp"
#include "../combat/UnitHitboxes.hpp"
#include "../combat/DestructableHitboxes.hpp"

// Third-person combat owns an animation/timeline, never a native pursuit order or CS ammunition.
class HeroMeleeAttack {
public:
    static bool Supported(wc3::Handle unit);
    void Reset() {timeline_.Reset();}
    void Cancel() {timeline_.Cancel();}
    bool Busy(double now) const {return timeline_.Busy(now);}
    bool Begin(wc3::Handle unit,double now,const GameplaySettings& settings,const UnitStatus& status);
    bool Tick(wc3::Handle unit,double now,float yaw,float feet,bool casting,
        const GameplaySettings& settings,const UnitStatus& status,UnitHitboxes& units,DestructableHitboxes& destructables);
private:
    bool Contact(wc3::Handle unit,float yaw,float feet,const GameplaySettings& settings,
        UnitHitboxes& units,DestructableHitboxes& destructables);
    HeroAttackTimeline timeline_;
};
