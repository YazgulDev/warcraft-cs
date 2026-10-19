#pragma once
#include "ViewmodelPose.hpp"
#include "../config/ViewmodelSettings.hpp"
#include <algorithm>
#include <cmath>

// A simulation-clock pose avoids render-rate-dependent bob and fades naturally on stops/airborne frames.
class ViewmodelMotion {
public:
    void Reset() {phase_=weight_=0;pose_={};}
    const ViewmodelPose& Pose() const {return pose_;}
    void Step(float dt,float speed,bool grounded,float duck,const ViewmodelSettings& settings) {
        if(!settings.bob || settings.bobAmount<=0){Reset();return;}
        if(!std::isfinite(dt) || dt<=0)return;
        dt=std::min(dt,.1f);
        float pace=std::isfinite(speed) ? std::clamp(speed/250.f,0.f,1.5f) : 0;
        float target=grounded ? pace*(duck>=.5f ? .45f : 1.f) : 0;
        weight_+=(target-weight_)*(1-std::exp(-settings.bobResponse*dt));
        if(target>0)phase_=std::fmod(phase_+dt*settings.bobFrequency*pace*6.283185307f,6.283185307f);
        float amount=settings.bobAmount*weight_;
        pose_={std::sin(phase_)*amount*.45f,std::cos(phase_*2)*amount*.3f,
            std::sin(phase_*2)*amount*.2f,std::sin(phase_)*amount*.7f};
    }
private:
    float phase_=0,weight_=0;
    ViewmodelPose pose_;
};
