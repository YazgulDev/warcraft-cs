#pragma once
#include "MovementPhysics.hpp"
#include <algorithm>
#include <cmath>

// Native ground pathing can nudge the logical actor even while the FPS body is airborne.
// Accept its corrected XY, but do not mistake a bounded local correction for a new ground spawn.
namespace MovementRelocation {
inline bool Rebase(MovementPhysics& motion,float floor,float distance=-1,float motorDistance=0) {
    constexpr float pathingCell=32,localCorrection=4*pathingCell;
    bool preserve=!motion.Grounded() && !motion.Swimming() && std::isfinite(motion.FeetZ()) &&
        std::isfinite(floor) && motion.FeetZ()>=floor-.01f && std::isfinite(distance) && distance>=0 &&
        std::isfinite(motorDistance) && distance<=std::max(0.f,motorDistance)+localCorrection;
    // Confirmed portals use the negative default; distant moves and buried destinations still reset.
    if(!preserve)motion.Reset(floor);
    return preserve;
}
}
