#pragma once
#include <cstdint>
// Explicit native oracle for disposable custom maps; ordinary builds contain no scene or file trigger.
namespace MovementScene { void Tick(uintptr_t base, const char* root); }
