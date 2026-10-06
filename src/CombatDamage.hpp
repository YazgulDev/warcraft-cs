#pragma once
#include <algorithm>

namespace CombatDamage {
// Friendly AWP uses its ordinary bullet damage; its enemy finishing rule must never bypass friendly fire.
inline bool FinishesTarget(bool awp, bool allied) { return awp && !allied; }
inline float Amount(float base, bool allied, bool finishing, float health) {
    return finishing ? std::max(health, 1.0f) * 100 + 1000 : base * (allied ? .5f : 1.0f);
}
}
