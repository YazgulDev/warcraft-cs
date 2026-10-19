#pragma once
#include "MovementObstacles.hpp"
#include "../combat/RocketProjectile.hpp"
#include "../combat/RocketVisual.hpp"
#include "../config/GameplaySettings.hpp"
#include "../audio/GameAudio.hpp"
#include <deque>

// Warcraft timer/target adapter owns launched rockets independently of the selected weapon and render context.
class NativeRockets {
public:
    void Configure(uintptr_t base){obstacles_.Configure(base);}
    void Reset();
    bool Launch(wc3::Handle actor,const GrenadePoint& eye,const GrenadePoint& direction,const GrenadePoint& motion,const GameplaySettings& settings);
    bool Tick(GameAudio& audio,wc3::Handle viewer,bool guide,const GrenadePoint& eye,const GrenadePoint& direction,float guideRange);
    bool Active(wc3::Handle actor) const;
    const std::vector<RocketVisual>& Visuals() const {return visuals_;}
    bool LaserVisible() const {return laserVisible_;}
    const GrenadePoint& Laser() const {return laser_;}
private:
    struct Trail {float time;GrenadePoint position;};
    struct Charge {RocketProjectile flight;wc3::Handle actor=0;int type=0;float born=0,damage=0,friendly=0;std::deque<Trail> trail;};
    struct Burst {GrenadePoint position;float born,size;};
    bool Clock();
    float Time() const;
    std::vector<Charge> charges_;
    std::vector<Burst> bursts_;
    std::vector<RocketVisual> visuals_;
    MovementObstacles obstacles_;
    wc3::Handle timer_=0;
    unsigned generation_=0;
    float nextLaser_=0;
    bool laserVisible_=false;
    GrenadePoint laser_{};
};
