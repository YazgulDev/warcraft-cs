#pragma once
#include "WarcraftApi.hpp"

// Live Warcraft collision is separate from visual scale, invulnerability and weapon hitboxes.
struct UnitCollisionProfile {
    bool known=false,colliding=true;
    float radius=0;
    static UnitCollisionProfile Read(wc3::Handle unit,uintptr_t gameBase);
};
