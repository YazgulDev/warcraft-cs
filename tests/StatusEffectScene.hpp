#pragma once
#include "../src/ShooterController.hpp"

// Compile only with -TestStatusEffects; fixtures never run in normal player builds.
namespace StatusEffectScene {
void Tick(const ShooterController& controller, uintptr_t gameBase, const char* root);
}
