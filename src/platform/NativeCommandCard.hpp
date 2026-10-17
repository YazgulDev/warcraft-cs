#pragma once
#include "../presentation/SpellHudSlot.hpp"
#include <cstdint>

// Match the real command button to this actor's ability before sampling its current icon/cost.
namespace NativeCommandCard {
bool Apply(uintptr_t ui,uintptr_t abilityObject,float mana,SpellHudSlot& slot);
bool PointOnly(uintptr_t base,uintptr_t ui,uintptr_t abilityObject,int ability,int order);
}
