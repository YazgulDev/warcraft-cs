#pragma once
#include <array>

// Visual-only controls never change damage, ray selection or the movement solver.
struct CombatVisualSettings {
    bool headshotBlood=true,headshotDecapitation=true,criticalText=true,shellCasings=true;
    // Denser, longer-lived head spray remains independent of damage and corpse ownership.
    float bloodSeconds=.8f,bloodScale=1.75f,criticalSeconds=1,criticalRise=40,criticalScale=1,bloodSpeed=220,bloodGravity=400,criticalOffset=24;
    unsigned bloodCount=48;
    float fatalBloodMultiplier=3;
    // Body impacts use a separate small spray and opt-out without weakening headshot feedback.
    bool bodyBlood=true;
    unsigned bodyBloodCount=6;
    float bodyBloodScale=.65f,bodyBloodSeconds=.35f,bodyBloodSpeed=110;
    float shellSeconds=3,shellSpeed=140,shellGravity=650,shellScale=1,shellBounce=.35f;
    unsigned shellMaximum=64;
    // Physical eye-relative right/up/forward offsets in Warcraft units, independent of screen resolution.
    std::array<float,3> shellOffset={10,-12,20};
};
