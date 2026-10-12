#pragma once

// Prototype tuning belongs to the existing INI snapshot and takes effect on F8.
struct ThirdPersonSettings {
    bool enabled=true;
    bool movementAnimations=true;
    bool autoAnimationSpeed=true;
    float animationSpeedMultiplier=1;
    float runCycleDistance=210, takeoffDuration=.10f, landingDuration=.16f;
    float distance=500, height=35, initialPitch=-12, clearance=16;
    float attackRange=150, attackArc=50, attackHeight=64, contactDelay=.56f, attackInterval=2.2f, damageMultiplier=1;
};
