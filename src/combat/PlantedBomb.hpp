#pragma once
#include "../platform/WarcraftApi.hpp"
#include "../audio/GameAudio.hpp"
#include "../platform/KillReward.hpp"
#include "BlastJumpEvent.hpp"

// One planted charge owns its simulation clock and world marker independently of equipped weapons.
class PlantedBomb {
public:
    // A fixed explosion lets high-health targets survive; it is no longer an unconditional kill.
    static constexpr float Damage = 2500.0f;
    bool Plant(wc3::Handle attacker, float x, float y, GameAudio& audio,float damage=Damage,float friendlyFirePercent=50,const BlastJumpSettings& jump={});
    bool Tick(GameAudio& audio,const KillReward::Sink& onKill={},BlastJumpEvents* jumps=nullptr,const std::function<bool()>& valid={});
    void Reset();
    bool Active() const { return timer_ && !exploded_; }
    bool Finished() const { return timer_==0; }
    float Remaining() const { return remaining_; }
private:
    wc3::Handle attacker_ = 0, marker_ = 0, timer_ = 0;
    float x_ = 0, y_ = 0, remaining_ = 0, nextBeep_ = 0;
    float damage_=Damage; // Snapshot planting settings; F8 cannot retroactively alter a live charge.
    float friendlyFirePercent_=50; // Allied damage follows the same planting snapshot as base damage.
    BlastJumpSettings jump_; // Plant-time movement response survives weapon switches/settings reloads.
    bool exploded_ = false;
};
