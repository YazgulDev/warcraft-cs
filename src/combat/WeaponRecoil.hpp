#pragma once
#include "GoldSrcRandom.hpp"

// Independent punch state keeps the camera and bullet ray coupled without overwriting mouse aim.
class WeaponRecoil {
public:
    void Reset();
    void ResetBurst();
    void Step(float seconds, bool triggerHeld, int weapon);
    void Shot(int weapon, float speed, bool grounded, bool ducked);
    float Pitch() const { return pitch_; }
    float Yaw() const { return yaw_; }
    float Magnitude() const;
    int Shots() const { return shots_; }
private:
    float pitch_ = 0, yaw_ = 0, recovery_ = 0;
    bool delayFire_ = false;
    int shots_ = 0;
    std::array<int, 2> direction_ = {-1, -1};
    GoldSrcRandom random_;
};
