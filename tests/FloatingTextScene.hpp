#pragma once
#include "../src/application/ShooterController.hpp"
namespace FloatingTextScene {
// Opt-in native oracle; no tags or units are created by ordinary client builds.
void Tick(ShooterController& controller, uintptr_t base, const char* root);
}
