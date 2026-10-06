#pragma once
#include "../src/ShooterController.hpp"
// Explicit offline fixtures are compiled only into the temporary native verification build.
namespace RuneCombatScene { void Tick(ShooterController& controller,uintptr_t base,const char* root); }
