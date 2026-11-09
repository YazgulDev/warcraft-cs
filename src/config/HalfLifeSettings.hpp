#pragma once
#include <array>

// Optional content is independently sealed; one missing weapon never disables another.
struct HalfLifeSettings {
    std::array<bool,3> enabled={true,true,true},available={false,false,false};
    struct Explosive {
        bool enabled=true,available=false;
        float radius,throwSpeed;
    };
    // Guest explosives keep their own launch and radius tuning while sharing the blast impulse policy.
    Explosive grenade{true,false,700,950},satchel{true,false,650,420};
    float handGrenadeFuse=2.5f;
    float tauChargeSeconds=4,tauDrainSeconds=.3f,tauChargedDamage=2000; // Stronger stock charge; the INI may override it.
    float tauSpinStartRate=1,tauSpinEndRate=3,tauPitchStart=1,tauPitchEnd=2.5f;
    int tauPrimaryCost=2;
    bool tauJumpRecoil=true;
    float tauPrimaryJumpSpeed=120,tauChargedJumpSpeed=800; // Opposite the accepted shot ray, GoldSrc units/s.
    float crowbarContactSeconds=.1f;
    bool crowbarSurfaceSounds=true;
    float beamSeconds=.15f,beamWidth=3;
    bool beamEffects=true,ownedSprites=true;
    float impactSize=18;
};
