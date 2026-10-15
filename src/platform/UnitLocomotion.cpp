#include "UnitLocomotion.hpp"
#include "NativeArchive.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

void UnitLocomotion::ObserveBaseRate() {
    float current=wc3::AnimationRate(unit_);
    // A map can change its own time scale during direct control; preserve that new baseline on release.
    if(!ownsRate_ || std::abs(current-lastRate_)>.01f)baselineRate_=current;
}
void UnitLocomotion::Rate(float value) {
    lastRate_=std::clamp(value,0.f,5.f);wc3::AnimationRate(unit_,lastRate_);ownsRate_=true;
}
void UnitLocomotion::Release() {
    if(ownsRate_ && unit_ && wc3::GetUnitTypeId(unit_)) {
        ObserveBaseRate();wc3::AnimationRate(unit_,baselineRate_);
    }
    ownsRate_=false;poseKnown_=false;
}
void UnitLocomotion::Update(wc3::Handle unit,double now,bool grounded,float speed,bool blocked,const ThirdPersonSettings& settings,bool walking) {
    int type=wc3::GetUnitTypeId(unit);
    // An impact callback can kill the actor before the next status snapshot; never replace its death pose with a gait.
    if(!type || wc3::Real(wc3::GetUnitState(unit,0))<=.405f) {Release();return;}
    auto path=unit_==unit && type_==type ? modelPath_ : wc3::UnitModelPath(type);
    if(unit_!=unit || type_!=type || modelPath_!=path) {
        // Morphs/map models invalidate cached gait metadata even when the native unit handle is unchanged.
        Release();Reset();unit_=unit;type_=type;modelPath_=path;
        auto model=path;auto extension=model.find_last_of('.');if(extension!=std::string::npos)model.resize(extension);
        auto metadata=NativeArchive::Read("Units\\UnitUI.slk");
        profile_.Load(NativeArchive::Read(model+".mdx"),std::string(metadata.begin(),metadata.end()),path,type);
        wc3::Log("Unit locomotion type=%08X walk=%s run=%s walkRef=%.1f runRef=%.1f",type,profile_.walk.name.c_str(),profile_.run.name.c_str(),profile_.walkSpeed,profile_.runSpeed);
    }
    auto next=motion_.Update(now,grounded,speed,settings,walking);
    if(blocked) {Release();return;}
    if(!settings.movementAnimations) {
        // An F8 opt-out stops our looping pose once; spell/attack owners never pass through this branch.
        bool owned=ownsRate_ || poseKnown_;Release();if(owned)wc3::Animate(unit,"stand");return;
    }
    ObserveBaseRate();
    using Pose=HeroMotionState::Pose;
    const auto& sequence=next==Pose::Run ? profile_.run : next==Pose::Walk ? profile_.walk :
        next==Pose::Takeoff ? profile_.takeoff : next==Pose::Air ? profile_.air : next==Pose::Land ? profile_.land : profile_.stand;
    int actual=wc3::AnimationSequence(unit);
    bool overwritten=poseKnown_ && sequence.index>=0 && actual>=0 && actual!=sequence.index;
    bool transition=!poseKnown_ || next!=pose_;
    if(transition || overwritten) {
        // Never request an Arthas-only pose from another model or restart its gait every frame.
        // Shift changes pace without resetting a shared Walk cycle; shared leap clips also continue naturally.
        // Native retaliation/movement can replace the sprite without changing our cached state.
        // Repair only a different live sequence; normal looping gaits never restart every frame.
        if(sequence.index>=0 && (!poseKnown_ || sequence.index!=sequence_ || overwritten))wc3::AnimateIndex(unit,sequence.index);
        sequence_=sequence.index;
        pose_=next;poseKnown_=true;
        if(transition)wc3::Log("Hero locomotion pose=%d grounded=%d speed=%.1f sequence=%s index=%d",int(next),grounded,speed,sequence.name.c_str(),sequence.index);
        // A persistent AI retry can overwrite every update; bounded telemetry keeps that repair from filling logs.
        if(overwritten && (lastRepair_==0 || GetTickCount()-lastRepair_>=1000)) {
            lastRepair_=GetTickCount();wc3::Trace("Hero locomotion repaired native sequence=%d expected=%d",actual,sequence.index);
        }
    }
    float rate=baselineRate_;
    if(next==Pose::Run || next==Pose::Walk) {
        // A shared Walk sequence keeps the same stride in both gaits; separate Run/Walk sequences use their own reference.
        float reference=next==Pose::Walk && profile_.walk.index!=profile_.run.index ? profile_.walkSpeed : profile_.runSpeed;
        rate=settings.autoAnimationSpeed && reference>0 ? speed/reference : speed*sequence.seconds/settings.runCycleDistance;
        rate*=settings.animationSpeedMultiplier;
    }
    if(next==Pose::Takeoff)rate=sequence.seconds/std::max(.02f,settings.takeoffDuration);
    if(next==Pose::Land)rate=sequence.seconds/std::max(.02f,settings.landingDuration);
    if(sequence.index<0 || baselineRate_==0)rate=baselineRate_; // Missing sequences and map freezes remain native.
    Rate(rate);
}
