#pragma once
#include <array>

// Optional content is independently sealed; one missing weapon never disables another.
struct HalfLifeSettings {
    std::array<bool,3> enabled={true,true,true},available={false,false,false};
    float tauChargeSeconds=4,tauDrainSeconds=.3f,tauChargedDamage=1000;
    float tauSpinStartRate=1,tauSpinEndRate=3,tauPitchStart=1,tauPitchEnd=2.5f;
    int tauPrimaryCost=2;
    float crowbarContactSeconds=.1f;
    bool crowbarSurfaceSounds=true;
    float beamSeconds=.15f,beamWidth=3;
    bool beamEffects=true,ownedSprites=true;
    float impactSize=18;
};
