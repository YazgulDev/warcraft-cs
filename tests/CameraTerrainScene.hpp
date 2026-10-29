#pragma once
#include <cstdint>

// Explicit disposable-map camera oracle; never compiled into normal Player modules.
namespace CameraTerrainScene {
void BeforeWorld(uintptr_t base,const char* root);
void AfterWorld();
bool BlocksMapCamera();
}
