#pragma once
#include "KillReward.hpp"
#include "../config/NuclearSettings.hpp"
#include "../audio/GameAudio.hpp"
#include <array>

// Native timer lifetime is independent of selected equipment and follows Warcraft pause/save time.
class NativeNuclearBomb {
public:
    bool Launch(wc3::Handle actor,const std::array<float,3>& center,const NuclearSettings& settings);
    void Tick(GameAudio& audio,const KillReward::Sink& rewards={});
    void Reset();
    float Remaining() const{return remaining_;}
    float Age() const{return age_;}
    float Flash() const;
    float Shake() const;
    const std::array<float,3>& Center() const{return center_;}
    bool Active() const{return timer_!=0;}
    const NuclearSettings& Settings() const{return settings_;}
private:
    void Detonate(const KillReward::Sink& rewards);
    wc3::Handle timer_=0,actor_=0,owner_=0;int actorType_=0;uintptr_t actorObject_=0;
    unsigned generation_=0;float remaining_=-1,age_=-1,nextBeep_=0;
    bool exploded_=false;std::array<float,3> center_{};NuclearSettings settings_;
};
