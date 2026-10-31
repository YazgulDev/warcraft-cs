#pragma once
#include "NativeRockets.hpp"
#include "NativeEffectService.hpp"
#include "../config/NuclearSettings.hpp"

// Shared RPG flight reports an impact; this adapter owns the subsequent native Warcraft effect sequence.
class NativeNuclearBomb {
public:
    void Configure(uintptr_t base){effects_.Configure(base);}
    bool Launch(wc3::Handle actor,const GrenadePoint& eye,const GrenadePoint& direction,const GrenadePoint& motion,const GameplaySettings& settings,NativeRockets& rockets);
    void Tick(GameAudio& audio,const KillReward::Sink& rewards={});
    void Reset();
    float Remaining() const{return -1;}
    float Age() const{return age_;}
    bool Flying() const{return timer_ && flying_;}
    float Flash() const;
    float Shake() const;
    float Exposure(const GrenadePoint& eye) const;
    const GrenadePoint& Center() const{return center_;}
    bool Active() const{return timer_!=0;}
    const NuclearSettings& Settings() const{return settings_;}
private:
    void Impact(const GrenadePoint& center);
    void Explosion();void Flames();void Smoke();
    wc3::Handle timer_=0,actor_=0,owner_=0;int actorType_=0;uintptr_t actorObject_=0;
    unsigned generation_=0;float impactTime_=0,age_=-1;
    bool flying_=false,exploded_=false,flames_=false,smoke_=false;
    GrenadePoint center_{};NuclearSettings settings_;NativeEffectService effects_;
};
