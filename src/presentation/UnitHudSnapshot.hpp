#pragma once
#include <array>
#include <string>
#include <cstdint>
#include "InventoryHudSlot.hpp"

// Rendering consumes copied values only; native unit/item pointers stay on the game thread.
struct UnitHudSnapshot {
    uint32_t actor=0;
    float health=0,maxHealth=0,mana=0,maxMana=0;
    bool hero=false,experienceKnown=false;
    int level=0,experience=0,levelExperience=0,nextExperience=0,inventorySize=0;
    std::array<InventoryHudSlot,6> items{};
};
