#pragma once

// Prototype tuning belongs to the existing INI snapshot and takes effect on F8.
struct ThirdPersonSettings {
    bool enabled=true;
    float distance=500, height=35, initialPitch=-12, clearance=16;
    float attackRange=150, attackArc=50, attackHeight=64, contactDelay=.56f, attackInterval=2.2f, damageMultiplier=1;
};
