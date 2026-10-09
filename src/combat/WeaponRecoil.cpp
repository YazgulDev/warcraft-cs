#include "WeaponRecoil.hpp"
#include <algorithm>
#include <cmath>

namespace {
struct Kick { float up, side, upGrowth, sideGrowth, upLimit, sideLimit; unsigned turnChance; };
// ReGameDLL CS classic unsilenced AK/M4 mechanics (MIT; notices in NOTICE/licenses).
// Warcraft pitch uses positive angles upward.
constexpr Kick ak[] = {
    {1.0f,.375f,.175f,.0375f,5.75f,1.75f,8},
    {1.5f,.45f,.225f,.05f,6.5f,2.5f,7},
    {2,1,.5f,.35f,9,6,5},
    {.9f,.35f,.15f,.025f,5.5f,1.5f,9}
};
constexpr Kick m4[] = {
    {.65f,.35f,.25f,.015f,3.5f,2.25f,7},
    {1,.45f,.28f,.045f,3.75f,3,7},
    {1.2f,.5f,.23f,.15f,5.5f,3.5f,6},
    {.6f,.3f,.2f,.0125f,3.25f,2,7}
};
}
void WeaponRecoil::Reset() {
    pitch_ = yaw_ = 0; ResetBurst(); direction_.fill(-1); random_.Reset(0xC516);
}
void WeaponRecoil::ResetBurst() {
    // CS reload clears the weapon's burst penalty, retaining the player's current punch angle.
    shots_ = 0; recovery_ = 0; delayFire_ = false;
}
float WeaponRecoil::Magnitude() const { return std::hypot(pitch_, yaw_); }
void WeaponRecoil::Step(float seconds, bool triggerHeld, int weapon) {
    if (seconds <= 0) return;
    float length = Magnitude();
    if (length > 0) {
        // Use PM_DropPunchAngle's per-command subtraction, rather than a smoothed exponential approximation.
        float remaining = std::max(0.0f, length - (10.0f + length * .5f) * seconds);
        pitch_ *= remaining / length; yaw_ *= remaining / length;
    }
    if (triggerHeld) return;
    // Classic ItemPostFrame caps a released rifle burst at 15 and starts its 400ms recovery timer.
    if (delayFire_) { delayFire_ = false; shots_ = std::min(shots_, 15); recovery_ = .4f; }
    else recovery_ -= seconds;
    if (weapon == 2) { shots_ = 0; return; } // CS pistols reset their shot count on release.
    // ItemPostFrame sheds at most one shot per command; a low frame rate must not catch up extra shots.
    if (shots_ > 0 && recovery_ < 0) { --shots_; recovery_ = .0225f; }
}
void WeaponRecoil::Shot(int weapon, float speed, bool grounded, bool ducked) {
    if (weapon < 0 || weapon > 3) return;
    // AWP has no rifle burst counter; USP adds punch but resets its count on trigger release.
    if (weapon != 3) ++shots_;
    if (weapon < 2) delayFire_ = true;
    if (weapon == 2 || weapon == 3) { pitch_ += 2; return; }
    // Match the source's stance precedence: moving first, stationary airborne, crouched, then standing.
    int stance = speed > 0 ? 1 : !grounded ? 2 : ducked ? 3 : 0;
    const Kick& kick = weapon == 0 ? ak[stance] : m4[stance];
    int& direction = direction_[weapon]; // CS stores a separate lateral direction on each rifle.
    float count = shots_ == 1 ? 0.0f : float(shots_);
    pitch_ = std::min(kick.upLimit, pitch_ + kick.up + count * kick.upGrowth);
    // Classic KickBack clamps only the direction of the new kick, including changes between stances.
    float side = yaw_ + direction * (kick.side + count * kick.sideGrowth);
    yaw_ = direction > 0 ? std::min(side, kick.sideLimit) : std::max(side, -kick.sideLimit);
    // Use the original inclusive RANDOM_LONG(0, chance) rule for lateral reversals.
    if (random_.Uniform(kick.turnChance) == 0) direction = -direction;
}
