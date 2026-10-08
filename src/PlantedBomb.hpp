#pragma once
#include "WarcraftApi.hpp"
#include "GameAudio.hpp"

// One planted charge owns its simulation clock and world marker independently of equipped weapons.
class PlantedBomb {
public:
    // A fixed explosion lets high-health targets survive; it is no longer an unconditional kill.
    static constexpr float Damage = 2500.0f;
    bool Plant(wc3::Handle attacker, float x, float y, GameAudio& audio,float damage=Damage,float friendlyFirePercent=50);
    bool Tick(GameAudio& audio);
    void Reset();
    bool Active() const { return timer_ && !exploded_; }
    bool Finished() const { return timer_==0; }
    float Remaining() const { return remaining_; }
private:
    wc3::Handle attacker_ = 0, marker_ = 0, timer_ = 0;
    float x_ = 0, y_ = 0, remaining_ = 0, nextBeep_ = 0;
    float damage_=Damage; // Snapshot planting settings; F8 cannot retroactively alter a live charge.
    float friendlyFirePercent_=50; // Allied damage follows the same planting snapshot as base damage.
    bool exploded_ = false;
};
