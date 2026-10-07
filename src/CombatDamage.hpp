#pragma once
#include <algorithm>

namespace CombatDamage {
// Friendly AWP uses its ordinary bullet damage; its enemy finishing rule must never bypass friendly fire.
inline bool FinishesTarget(bool awp, bool allied) { return awp && !allied; }
inline float Amount(float base, bool allied, bool finishing, float health, float friendlyFirePercent=50) {
    // Apply allied scaling first, so even an erroneous finishing flag cannot bypass friendly protection.
    if (allied) return base * (std::clamp(friendlyFirePercent,0.0f,100.0f) / 100.0f);
    return finishing ? std::max(health, 1.0f) * 100 + 1000 : base;
}
}
