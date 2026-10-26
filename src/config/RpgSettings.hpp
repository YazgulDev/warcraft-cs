#pragma once

// Warcraft-scaled Half-Life-style rocket tuning; every flight retains its launch snapshot across F8.
struct RpgSettings {
    bool enabled=true,available=false,guidance=true;
    // Wider default blast; launched rockets retain their configured radius across reloads.
    float radius=600,launchSpeed=375,maximumSpeed=3000,acceleration=6000;
    float ignition=.4f,boostSeconds=1,gravity=600,lifetime=12,inheritedVelocity=1;
    float modelScale=1.5f,trailSeconds=1,trailSize=10;
    unsigned maxActive=16;
};
