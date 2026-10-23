#pragma once
#include <cstdint>
#include <initializer_list>

#include "GameBuildKind.hpp"

namespace wc3 {
// Warcraft III builds differ in every hard-coded address: the JASS native table is
// recompiled, the engine internals are re-laid out, and float code generation moved
// from x87 to SSE. Only the object layouts survived. Addresses therefore live in a
// per-build translation table instead of being baked into each adapter.
// Classify the loaded Game.dll. Sets the active build used by Rva().
GameBuildKind SelectBuild(void* module);
// Isolated adapter fixtures default to the legacy layout; SelectBuild always classifies the real host first.
inline GameBuildKind activeBuild = GameBuildKind::Classic126a;
inline GameBuildKind ActiveBuild() { return activeBuild; }
const char* ActiveBuildName();
// True once Bind() accepted the module and the native table has been populated.
bool IsBound();
// Recorded by Bind() after every native pointer has been installed.
void MarkBound();

// Translate a 1.26a relative virtual address into the active build's RVA.
// Returns 0 when this build has no verified address for that legacy offset. A zero
// result is a deliberate "feature unavailable" signal: callers must not fall back to
// the legacy value, because 1.26a and 1.27a reuse the same numeric ranges for
// completely different functions.
inline uintptr_t Rva(uintptr_t legacy126a) {
    if (activeBuild == GameBuildKind::Classic126a) return legacy126a;
    if (activeBuild != GameBuildKind::Classic127a) return 0;
    switch (legacy126a) {
#define BUILD_ADDRESS(legacy, translated) case legacy: return translated;
#include "GameBuild127a.inc"
#undef BUILD_ADDRESS
        default: return 0;
    }
}
// True when every listed legacy RVA resolves in the active build.
inline bool RvaAvailable(std::initializer_list<uintptr_t> offsets) {
    for (auto offset : offsets) if (!Rva(offset)) return false;
    return true;
}
}
