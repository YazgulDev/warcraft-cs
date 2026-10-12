#pragma once
#include "WarcraftApi.hpp"
#include "../presentation/HeroMotionState.hpp"
#include "../presentation/ModelMotionProfile.hpp"

// All controlled unit models supply their own gait; native casts and melee retain animation ownership.
class UnitLocomotion {
public:
    void Update(wc3::Handle unit,double now,bool grounded,float speed,bool blocked,
        const ThirdPersonSettings& settings,bool walking=false);
    void Release();
    void Reset() {unit_=0;type_=0;ownsRate_=false;poseKnown_=false;motion_.Reset();modelPath_.clear();}
private:
    void Rate(float value);
    void ObserveBaseRate();
    wc3::Handle unit_=0;
    float baselineRate_=1,lastRate_=1;
    int type_=0;
    int sequence_=-1;
    std::string modelPath_;
    ModelMotionProfile profile_;
    bool ownsRate_=false,poseKnown_=false;
    HeroMotionState::Pose pose_=HeroMotionState::Pose::Stand;
    HeroMotionState motion_;
};
