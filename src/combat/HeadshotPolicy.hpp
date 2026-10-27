#pragma once
#include "WeaponSlots.hpp"
#include "../config/HitDetectionSettings.hpp"

namespace HeadshotPolicy {
// A verified direct head contact can emit blood even when the optional damage bonus is disabled.
inline bool Direct(int weapon,bool head) {
    return head && ((WeaponSlots::Firearm(weapon) && weapon!=WeaponSlots::RPG) || WeaponSlots::Energy(weapon));
}
inline bool Accepted(int weapon,bool head,const HitDetectionSettings& settings) {
    return Direct(weapon,head) && settings.headshots && (!WeaponSlots::Energy(weapon) || settings.energyHeadshots);
}
inline float Damage(float base,bool accepted,const HitDetectionSettings& settings) {
    return accepted ? base*settings.headshotMultiplier : base;
}
}
