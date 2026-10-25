#pragma once

// GoldSrc units are converted only by physics; collision uses Warcraft world units.
struct MovementSettings {
    // Classic restores the pre-platform-recovery collision behavior; recovery remains an opt-in mode.
    bool platformRecovery=false;
    // Live creature footprints replace oversized visual extents; solid buildings/trees keep model geometry.
    bool useNativeUnitCollision=true;
    // Precise stationary prop surfaces keep arch openings and low column bases usable.
    bool objectMeshCollision=true;
    // Repair invalid buried states; manual recovery returns only to revalidated visited ground.
    bool recoverInvalidPosition=true;
    float recoveryDistance=512;
    // Optional absolute unit-speed scaling replaces the legacy relative slow/haste modifier.
    bool scaleFromUnitMoveSpeed=false;
    float unitMoveSpeedReference=300;
    bool bunnyHop = true;
    bool autoJump = true;
    // Classic strafing supplies the default gain; a separate arcade takeoff boost remains optional.
    float jumpBoostPercent = 0;
    float maxBunnySpeed = 1000;
    float airAcceleration = 10;
    float jumpSpeed = 268.328f;
    float gravity = 800;
    float stepHeight = 27;
    // Local slope classification and a capped player hull keep Warcraft-sized actors out of narrow-pass checks.
    float maxWalkSlope = 46;
    float hullRadius = 24;
};
