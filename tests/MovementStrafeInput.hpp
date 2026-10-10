#pragma once
#include "../src/movement/MovementPhysics.hpp"
#include <cmath>

// Verification-only input models smooth A/left, D/right turns aligned with current travel.
// Production never steers the player's camera or presses strafe keys automatically.
inline MoveInput MovementStrafeInput(const MovementPhysics& movement, float elapsed, float initialYaw = 0) {
    MoveInput input;
    input.jump = true;
    input.right = int(elapsed / 0.1f) % 2 ? 1.0f : -1.0f;
    input.yaw = movement.Speed() > 0.01f ?
        std::atan2(movement.VelocityY(), movement.VelocityX()) * 57.295779513f : initialYaw;
    return input;
}
