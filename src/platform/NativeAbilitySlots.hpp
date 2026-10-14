#pragma once
#include <array>
#include <cstdint>
#include <vector>

// Actual learned ability instances, sampled without retaining pointers beyond this call.
namespace NativeAbilitySlots {
std::array<int,4> Read(uintptr_t base,uintptr_t unit);
// Squad units need their full live list, including non-hero command rows and map-added abilities.
std::vector<int> All(uintptr_t base,uintptr_t unit);
}
