#pragma once

// Each launched grenade snapshots tuning; F8 changes subsequent throws, never a live fuse.
struct GrenadeSettings {
    bool enabled=true;
    float throwSpeed=1125,gravity=660,bounce=.45f,inheritedVelocity=1;
    float lightGravityScale=10.f/11; // Flash/smoke use CS's .5 gravity versus HE's .55.
    float fuse=1.5f,releaseDelay=.25f,recovery=.8f;
    float heRadius=350,flashRadius=1000,flashSeconds=3,flashUnitStunSeconds=2;
    // Flash disorients every exposed creature by default, including allies and the controlled actor.
    bool flashStunAllUnits=true,flashStunDistanceFalloff=false;
    float smokeRadius=300,smokeSeconds=18,smokeOpacity=.85f;
    unsigned maxActive=32;
    float worldModelScale=1,worldSpin=360;
};
