#pragma once
#include "WarcraftApi.hpp"
#include "../movement/MovementPhysics.hpp"
#include <algorithm>

namespace AirbornePathing {
inline bool CanOverfly(float x, float y, float terrain, const MovementPhysics& movement) {
    // The walking mask has no height. Allow a high crossing only after the caller
    // has checked solid geometry, and only where the native flight mask permits it.
    constexpr int Flyability = 2; // PATHING_TYPE_FLYABILITY in common.j.
    const float lowestFeet = std::min(movement.FeetZ(), movement.PreviousFeetZ());
    return !movement.Grounded() && !movement.Swimming() && !movement.Surfing() &&
        lowestFeet - terrain >= movement.BodyHeight() &&
        !wc3::IsTerrainPathable(&x, &y, Flyability);
}
}
