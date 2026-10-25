#pragma once
#include <array>

// Visual-only controls never change damage, ray selection or the movement solver.
struct CombatVisualSettings {
    bool headshotBlood=true,criticalText=true,shellCasings=true;
    float bloodSeconds=.5f,bloodScale=1,criticalSeconds=1,criticalRise=40,criticalScale=1,bloodSpeed=180,bloodGravity=400,criticalOffset=24;
    unsigned bloodCount=16;
    float shellSeconds=3,shellSpeed=140,shellGravity=650,shellScale=1,shellBounce=.35f;
    unsigned shellMaximum=64;
    // Physical eye-relative right/up/forward offsets in Warcraft units, independent of screen resolution.
    std::array<float,3> shellOffset={10,-12,20};
};
