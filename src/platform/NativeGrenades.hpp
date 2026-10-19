#pragma once
#include "MovementObstacles.hpp"
#include "../combat/GrenadeProjectile.hpp"
#include "../combat/GrenadeVisual.hpp"
#include "../config/GameplaySettings.hpp"
#include "../audio/GameAudio.hpp"

// Native time/targets are isolated from the portable trajectory and the read-only render snapshots.
class NativeGrenades {
public:
    void Configure(uintptr_t base) {obstacles_.Configure(base);}
    void Reset();
    bool Throw(wc3::Handle actor,int slot,const GrenadePoint& eye,const GrenadePoint& direction,const GrenadePoint& motion,const GameplaySettings& settings);
    bool Tick(GameAudio& audio,wc3::Handle viewer,const GrenadePoint& eye,const GrenadePoint& direction);
    const std::vector<GrenadeVisual>& Visuals() const {return visuals_;}
    float FlashOpacity() const {return flashOpacity_;}
private:
    struct Charge {GrenadeProjectile flight;wc3::Handle actor=0;int type=0,slot=0;float born=0,damage=0,friendly=0;bool detonated=false;};
    struct Stun {wc3::Handle unit;int type;float until;};
    float Time() const;
    bool Detonate(const Charge& charge,float now,GameAudio& audio,wc3::Handle viewer,const GrenadePoint& eye,const GrenadePoint& direction);
    bool Covered(const GrenadePoint& from,const GrenadePoint& to,const std::vector<Bounds3>& obstacles) const;
    std::vector<Charge> charges_;
    std::vector<Stun> stuns_;
    std::vector<GrenadeVisual> visuals_;
    MovementObstacles obstacles_;
    wc3::Handle timer_=0;
    unsigned generation_=0;
    float flashStart_=0,flashDuration_=0,flashOpacity_=0;
};
