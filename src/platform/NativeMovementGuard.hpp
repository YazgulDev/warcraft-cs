#pragma once
#include "FpsCombatGuard.hpp"

// Suppresses Warcraft's walking motor without changing saved speed/root counters or scripted placement.
class NativeMovementGuard {
public:
    static void Install(uintptr_t base,wc3::Handle (*actor)(),bool (*casting)(),FpsCombatGuard::HookInstaller hook,
        wc3::Handle (*airborneActor)()=nullptr);
};
