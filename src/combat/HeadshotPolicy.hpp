#pragma once
#include "WeaponSlots.hpp"
#include "../config/HitDetectionSettings.hpp"

namespace HeadshotPolicy {
// Only a named head surface on a direct firearm contact earns the configured bonus.
inline bool Accepted(int weapon,bool head,const HitDetectionSettings& settings) {
    return head && settings.headshots && WeaponSlots::Firearm(weapon) && weapon!=WeaponSlots::RPG;
}
inline float Damage(float base,bool accepted,const HitDetectionSettings& settings) {
    return accepted ? base*settings.headshotMultiplier : base;
}
}
