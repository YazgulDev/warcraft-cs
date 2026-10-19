#pragma once
#include "GrenadeProjectile.hpp"
#include <vector>

// Immutable render data contains no native handles; flight and trail survive weapon/view changes.
struct RocketVisual {
    GrenadePoint position{},direction{};
    std::vector<GrenadePoint> trail;
    float scale=1.5f,trailSize=10;
    bool powered=false;
    bool explosion=false;
    float age=0;
};
