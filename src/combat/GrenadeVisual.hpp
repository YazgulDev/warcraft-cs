#pragma once
#include "GrenadeProjectile.hpp"

// A read-only effect snapshot lets presentation draw flights/clouds without owning native handles or clocks.
struct GrenadeVisual {
    GrenadePoint position{};
    int slot=0;
    float radius=4,opacity=1,age=0;
    bool smoke=false;
};
