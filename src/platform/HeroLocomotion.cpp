#include "HeroLocomotion.hpp"
#include "NativeArchive.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
float WalkDuration(wc3::Handle unit) {
    auto path=wc3::UnitModelPath(wc3::GetUnitTypeId(unit));
    path=path.substr(0,path.find(','));auto extension=path.find_last_of('.');
    if(extension!=std::string::npos)path.resize(extension);
    auto bytes=NativeArchive::Read(path+".mdx");
    // Read the owner's active model, including map imports; no animation assets enter the public build.
    if(bytes.size()<4 || std::memcmp(bytes.data(),"MDLX",4))return .766f;
    for(size_t at=4;at+8<=bytes.size();) {
        uint32_t length=0;std::memcpy(&length,bytes.data()+at+4,4);size_t body=at+8;
        if(length>bytes.size()-body)break;
        if(!std::memcmp(bytes.data()+at,"SEQS",4))for(size_t seq=body;seq+132<=body+length;seq+=132) {
            if(_strnicmp(reinterpret_cast<const char*>(bytes.data()+seq),"Walk",4))continue;
            uint32_t begin=0,end=0;std::memcpy(&begin,bytes.data()+seq+80,4);std::memcpy(&end,bytes.data()+seq+84,4);
            if(end>begin && end-begin<=10000)return float(end-begin)*.001f;
        }
        at=body+length;
    }
    return .766f;
}
}
void HeroLocomotion::ObserveBaseRate() {
    float current=wc3::AnimationRate(unit_);
    // A map can change its own time scale during direct control; preserve that new baseline on release.
    if(!ownsRate_ || std::abs(current-lastRate_)>.01f)baselineRate_=current;
}
void HeroLocomotion::Rate(float value) {
    lastRate_=std::clamp(value,0.f,5.f);wc3::AnimationRate(unit_,lastRate_);ownsRate_=true;
}
void HeroLocomotion::Release() {
    if(ownsRate_ && unit_ && wc3::GetUnitTypeId(unit_)) {
        ObserveBaseRate();wc3::AnimationRate(unit_,baselineRate_);
    }
    ownsRate_=false;poseKnown_=false;
}
void HeroLocomotion::Update(wc3::Handle unit,double now,bool grounded,float speed,bool blocked,const ThirdPersonSettings& settings) {
    if(unit_!=unit) {Release();Reset();unit_=unit;walkSeconds_=WalkDuration(unit);}
    auto next=motion_.Update(now,grounded,speed,settings);
    if(blocked) {Release();return;}
    if(!settings.movementAnimations) {
        // An F8 opt-out stops our looping pose once; spell/attack owners never pass through this branch.
        bool owned=ownsRate_ || poseKnown_;Release();if(owned)wc3::Animate(unit,"stand");return;
    }
    ObserveBaseRate();
    if(!poseKnown_ || next!=pose_) {
        // Stock Arthas has no Jump sequence: use its authored hit/ready poses for the three leap phases.
        const char* sequence=next==HeroMotionState::Pose::Run ? "walk" :
            next==HeroMotionState::Pose::Air ? "stand ready" :
            next==HeroMotionState::Pose::Takeoff || next==HeroMotionState::Pose::Land ? "stand hit" : "stand";
        wc3::Animate(unit,sequence);pose_=next;poseKnown_=true;
        wc3::Log("Hero locomotion pose=%d grounded=%d speed=%.1f",int(next),grounded,speed);
    }
    // One walk cycle covers the configurable stride; actual slowed/accelerated movement drives the feet.
    float rate=next==HeroMotionState::Pose::Run ? speed*walkSeconds_/settings.runCycleDistance : baselineRate_;
    if(next==HeroMotionState::Pose::Takeoff)rate=.5f/std::max(.02f,settings.takeoffDuration);
    if(next==HeroMotionState::Pose::Land)rate=.5f/std::max(.02f,settings.landingDuration);
    if(baselineRate_==0)rate=0; // Preserve a map's explicit animation freeze rather than restarting its actor.
    Rate(rate);
}
