#pragma once
#include "../config/GameplaySettings.hpp"
#include <vector>

namespace WeaponDropRules {
// Uniformly choose an enabled firearm; explosives, melee and missing optional RPG assets are excluded.
inline std::vector<int> Candidates(const GameplaySettings& settings) {
    std::vector<int> result;
    if(!settings.warcraftOnly)for(int slot=0;slot<WeaponSlots::Count;++slot)
        if(WeaponSlots::Firearm(slot) && settings.WeaponEnabled(slot))result.push_back(slot);
    return result;
}
inline bool Success(const WeaponDropSettings& settings,float percentRoll) {
    return settings.enabled && settings.chancePercent>0 && percentRoll>=0 && percentRoll<settings.chancePercent;
}
}
