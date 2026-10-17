#pragma once

// Prototype tuning belongs to the existing INI snapshot and takes effect on F8.
struct ThirdPersonSettings {
    bool enabled=true;
    bool movementAnimations=true;
    bool showTargetIndicator=true;
    float targetCircleRadius=64;
    bool autoAnimationSpeed=true;
    float animationSpeedMultiplier=1;
    float runCycleDistance=210, takeoffDuration=.10f, landingDuration=.16f;
    float distance=500, height=35, initialPitch=-12, clearance=16;
    // Building model boxes include empty porches/overhangs; keep zoom unless explicitly opted in.
    bool buildingCollision=false;
    // Wheel changes a session distance, bounded independently of the native collision boom.
    float zoomStep=50, minimumDistance=80, maximumDistance=1000;
    float attackRange=150, attackArc=50, attackHeight=64, contactDelay=.56f, attackInterval=2.2f, damageMultiplier=1;
    // Ranged heroes use live map statistics by default; the manual range also covers unavailable native metadata.
    bool useNativeAttackRange=true, nativeRangedProjectiles=true;
    float rangedAttackRange=600,rangedRangeMultiplier=1,meleeHeightTolerance=32;
};
