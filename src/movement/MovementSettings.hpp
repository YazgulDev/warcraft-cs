#pragma once
#include "SwimmingSettings.hpp"

// GoldSrc units are converted only by physics; collision uses Warcraft world units.
struct MovementSettings {
    SwimmingSettings swimming;
    // Classic is the requested default; recovery and hull remain selectable without resetting motion.
    bool classicCollision=true;
    bool classicRampContacts=true; // Airborne contact with a verified walkable ramp becomes floor contact, not an axis stop.
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
    // Faster default takeoff/air gain; configurable zero boost retains pure classic strafing.
    float jumpBoostPercent = 15;
    float maxBunnySpeed = 3000; // GoldSrc units/s; horizontal world-speed cap is scaled once by physics.
    float airAcceleration = 30;
    float jumpSpeed = 268.328f;
    float gravity = 800;
    float stepHeight = 27;
    // Local slope classification and a capped player hull keep Warcraft-sized actors out of narrow-pass checks.
    float maxWalkSlope = 46;
    float hullRadius = 24;
};
