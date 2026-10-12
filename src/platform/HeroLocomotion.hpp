#pragma once
#include "WarcraftApi.hpp"
#include "../presentation/HeroMotionState.hpp"

// The animation adapter yields to native casting/incapacitation and the melee windup owner.
class HeroLocomotion {
public:
    void Update(wc3::Handle unit,double now,bool grounded,float speed,bool blocked,
        const ThirdPersonSettings& settings);
    void Release();
    void Reset() {unit_=0;ownsRate_=false;poseKnown_=false;motion_.Reset();}
private:
    void Rate(float value);
    void ObserveBaseRate();
    wc3::Handle unit_=0;
    float baselineRate_=1,lastRate_=1,walkSeconds_=.766f;
    bool ownsRate_=false,poseKnown_=false;
    HeroMotionState::Pose pose_=HeroMotionState::Pose::Stand;
    HeroMotionState motion_;
};
