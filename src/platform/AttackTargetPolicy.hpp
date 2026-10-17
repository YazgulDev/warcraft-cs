#pragma once
#include "WarcraftApi.hpp"

namespace AttackTargetPolicy {
// Neutral-passive ownership is not a player ally, even when Warcraft's alliance bit says otherwise.
inline bool Allied(wc3::Handle unit,wc3::Handle player) {
    return !wc3::IsNeutralUnit(unit) && wc3::IsUnitAlly(unit,player);
}
inline bool FinishingAllowed(wc3::Handle unit,wc3::Handle player) {
    // Newly targetable neutral-passive scenery keeps native quest immunity even with AWP finishing enabled.
    return !wc3::IsNeutralUnit(unit) || wc3::IsUnitEnemy(unit,player);
}
}
