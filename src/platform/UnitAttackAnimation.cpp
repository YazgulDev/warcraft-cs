#include "UnitAttackAnimation.hpp"
#include "NativeArchive.hpp"
#include <cmath>

void UnitAttackAnimation::Release() {
    if(owned_ && unit_ && wc3::GetUnitTypeId(unit_)) {
        // A map-owned freeze/time-scale change wins over the temporary manual attack rate.
        float current=wc3::AnimationRate(unit_);
        if(std::abs(current-pose_.Rate())>.01f)baseline_=current;
        wc3::AnimationRate(unit_,baseline_);
        wc3::AnimationBlending(unit_,baselineBlending_);
    }
    owned_=false;pose_.Reset();
}
void UnitAttackAnimation::Begin(wc3::Handle unit,double now,float speed,float contact,float interval) {
    Release();int type=wc3::GetUnitTypeId(unit);auto path=wc3::UnitModelPath(type);
    if(unit_!=unit || type_!=type || path_!=path) {
        unit_=unit;type_=type;path_=path;
        auto model=path;auto extension=model.find_last_of('.');if(extension!=std::string::npos)model.resize(extension);
        profile_.Load(NativeArchive::Read(model+".mdx"),"",path,type);
    }
    // Movement can replace Attack each frame: repeatedly blending from Stand hides the later swing frames.
    // Temporarily use the authored pose directly; restore the model's exact prior blending policy on release.
    baseline_=wc3::AnimationRate(unit);baselineBlending_=wc3::AnimationBlending(unit);
    wc3::AnimationBlending(unit,false);wc3::Animate(unit,"attack");
    auto clip=profile_.SequenceAt(wc3::AnimationSequence(unit));
    auto name=ModelMotionProfile::Lower(clip.name);
    // Retain the native random/alternate attack variation, using a real authored fallback if it was reset immediately.
    if(name!="attack" && name.rfind("attack ",0)!=0)clip=profile_.attack;
    sequence_=clip.index;
    float scale=std::isfinite(speed) ? std::clamp(speed,.2f,5.f) : 1;
    pose_.Begin(now,sequence_>=0 ? clip.seconds : contact,std::clamp(baseline_*scale,0.f,10.f),std::min(contact,interval)/scale,interval/scale);
    owned_=true;if(sequence_>=0)wc3::AnimateIndex(unit,sequence_);
    wc3::AnimationRate(unit,pose_.Rate());
    wc3::Log("Manual attack pose unit=%08X sequence=%d clip=%.3f rate=%.3f",unit,sequence_,clip.seconds,pose_.Rate());
}
void UnitAttackAnimation::Update(double now) {
    if(!owned_)return;
    if(!pose_.Active(now) || !unit_ || wc3::GetUnitTypeId(unit_)!=type_ || wc3::Real(wc3::GetUnitState(unit_,0))<=.405f) {Release();return;}
    // Do not overwrite a new map-owned time scale while a swing is in progress.
    if(std::abs(wc3::AnimationRate(unit_)-pose_.Rate())>.01f) {Release();return;}
    // Relocation/native impact cleanup may reset Stand or even restart the same Attack sequence.
    // Restore authored clip time, not frame zero, so running attacks keep advancing smoothly.
    if(sequence_>=0) {
        if(wc3::AnimationSequence(unit_)!=sequence_)wc3::AnimateIndex(unit_,sequence_);
        wc3::AnimationSeek(unit_,sequence_,pose_.Elapsed(now));
    }
    wc3::AnimationRate(unit_,pose_.Rate());
}
