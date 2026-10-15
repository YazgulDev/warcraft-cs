#pragma once
#include "../presentation/SpellHudSlot.hpp"
#include <cstdint>

// Read live map overrides and the simulation's cooldown clock without changing ability state.
namespace NativeAbilityInfo {
void Read(uintptr_t ability,SpellHudSlot& slot,float mana=-1);
bool Definition(uintptr_t base,int ability,int& column,int& row,std::string& icon);
// Some autocast abilities have a dynamic button getter; their verified native RTTI identifies a manual cast form.
std::string ClassName(uintptr_t ability);
}
