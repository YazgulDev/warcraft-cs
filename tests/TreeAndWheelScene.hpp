#pragma once
#include "../src/application/ShooterController.hpp"

// Explicit file-driven fixtures are compiled only into the disposable native verification build.
namespace TreeAndWheelScene { void Tick(ShooterController& controller,uintptr_t base,const char* root); }
