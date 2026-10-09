#pragma once
#include <cstdint>

// Native camera interception belongs to presentation; the controller supplies only its ownership state.
namespace MapCameraGuard {
using HookInstaller = bool (*)(void*, void*, void**);
void Install(uintptr_t base, bool (*blocked)(), HookInstaller hook);
// Place the FPS target beyond RTS limits without persisting expanded mission/minimap bounds.
void PositionEyeTarget(float* x, float* y);
}
