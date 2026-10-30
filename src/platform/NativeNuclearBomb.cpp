#include "NativeNuclearBomb.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

void NativeNuclearBomb::Reset(){++generation_;timer_=actor_=owner_=0;actorType_=0;actorObject_=0;remaining_=age_=-1;exploded_=false;nextBeep_=0;}
bool NativeNuclearBomb::Launch(wc3::Handle actor,const std::array<float,3>& center,const NuclearSettings& settings){
    if(timer_ || !settings.enabled || !wc3::SinglePlayer() || !wc3::GetUnitTypeId(actor))return false;
    auto clock=wc3::CreateTimer();if(!clock)return false;float duration=360000;wc3::TimerStart(clock,&duration,FALSE,0);
    timer_=clock;actor_=actor;actorType_=wc3::GetUnitTypeId(actor);actorObject_=wc3::UnitObject(actor);owner_=wc3::GetOwningPlayer(actor);
    center_=center;settings_=settings;remaining_=settings.delay;age_=-1;exploded_=false;nextBeep_=0;
    wc3::Log("Nuclear bomb launched actor=%08X delay=%.2f radius=%.0f buildings=%d",actor,settings.delay,settings.radius,settings.buildings);return true;
}
void NativeNuclearBomb::Detonate(const KillReward::Sink& rewards){
    unsigned generation=generation_;auto actor=actor_,owner=owner_;int type=actorType_;auto object=actorObject_;auto settings=settings_;auto center=center_;
    auto valid=[&]{return generation_==generation && wc3::GetUnitTypeId(actor)==type && wc3::UnitObject(actor)==object && wc3::GetOwningPlayer(actor)==owner;};
    if(!valid()){wc3::Log("Nuclear bomb damage cancelled: launch actor removed or transferred");return;}
    struct Victim {wc3::Handle unit;int type;uintptr_t object;};std::vector<Victim> victims;
    auto group=wc3::CreateGroup();if(!group){wc3::LogError("Nuclear bomb enumeration allocation failed");return;}
    // Enumerate all sixteen classic owner slots, including neutral hostile, before death scripts can mutate them.
    for(int slot=0;slot<16;++slot){wc3::GroupEnumUnitsOfPlayer(group,wc3::Player(slot),0);
        for(auto unit=wc3::FirstOfGroup(group);unit;unit=wc3::FirstOfGroup(group)){
            wc3::GroupRemoveUnit(group,unit);victims.push_back({unit,wc3::GetUnitTypeId(unit),wc3::UnitObject(unit)});
        }
    }wc3::DestroyGroup(group);unsigned killed=0,protectedUnits=0;
    for(const auto& victim:victims){
        if(!valid())return;auto target=victim.unit;
        if(target==actor || !victim.type || wc3::GetUnitTypeId(target)!=victim.type || wc3::UnitObject(target)!=victim.object ||
            !wc3::IsUnitEnemy(target,owner) || wc3::IsUnitAlly(target,owner) || wc3::IsUnitHidden(target) || wc3::IsUnitLoaded(target) ||
            (!settings.buildings && wc3::IsUnitType(target,2)))continue;
        float before=wc3::Real(wc3::GetUnitState(target,0));if(!std::isfinite(before) || before<=.405f)continue;
        if(settings.radius>0 && std::hypot(wc3::Real(wc3::GetUnitX(target))-center[0],wc3::Real(wc3::GetUnitY(target))-center[1])>settings.radius)continue;
        // Respect explicit quest invulnerability; native universal damage retains damage/death triggers and XP.
        if(wc3::GetUnitAbilityLevel(target,0x4176756C)>0){++protectedUnits;continue;}
        auto sink=settings.rewards ? rewards : KillReward::Sink{};auto reward=KillReward::Capture(actor,target,sink);
        float maximum=wc3::Real(wc3::GetUnitState(target,1));if(!std::isfinite(maximum))maximum=before;
        float damage=std::clamp(std::max(before,maximum)*4.f,1000000.f,1e12f);
        wc3::UnitDamageTarget(actor,target,&damage,FALSE,TRUE,5,26,0);
        if(!valid())return;reward.Confirm(sink);if(!valid())return;
        float after=wc3::GetUnitTypeId(target) ? wc3::Real(wc3::GetUnitState(target,0)) : 0;
        if(after<=.405f)++killed;else ++protectedUnits;
        wc3::Log("Nuclear bomb victim=%08X hpBefore=%.1f hpAfter=%.1f",target,before,after);
    }
    wc3::Log("Nuclear bomb detonated killed=%u protectedOrScripted=%u",killed,protectedUnits);
}
void NativeNuclearBomb::Tick(GameAudio& audio,const KillReward::Sink& rewards){
    if(!timer_)return;float elapsed=wc3::Real(wc3::TimerGetElapsed(timer_));remaining_=exploded_ ? -1 : std::max(0.f,settings_.delay-elapsed);
    if(!exploded_ && elapsed<settings_.delay){if(elapsed>=nextBeep_){nextBeep_=elapsed+1;audio.Play("c4_beep5.wav",.55f);}return;}
    age_=std::max(0.f,elapsed-settings_.delay);
    if(!exploded_){
        exploded_=true;remaining_=-1;audio.Play("c4_explode1.wav",1);unsigned generation=generation_;
        auto effect=wc3::Effect("Objects\\Spawnmodels\\Other\\NeutralBuildingExplosion\\NeutralBuildingExplosion.mdl",center_[0],center_[1]);if(effect)wc3::DestroyEffect(effect);
        if(generation_!=generation)return;Detonate(rewards);if(generation_!=generation)return;
    }
    if(age_>=settings_.effectSeconds){wc3::DestroyTimer(timer_);timer_=0;age_=-1;}
}
float NativeNuclearBomb::Flash() const{return age_>=0 && settings_.flashSeconds>0 ? .85f*std::clamp(1-age_/settings_.flashSeconds,0.f,1.f) : 0;}
float NativeNuclearBomb::Shake() const{return age_>=0 && age_<1.5f ? std::sin(age_*71)*settings_.shakeDegrees*(1-age_/1.5f) : 0;}
