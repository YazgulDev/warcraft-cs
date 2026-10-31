#pragma once
#include <string>
#include <algorithm>

// The code-only charge uses shared RPG flight and a bounded impact, never purchases/refills/random loot.
struct NuclearSettings {
    bool enabled=true,repeatCodes=false,buildings=true,rewards=true;
    bool neutralBuildings=true,neutralUnits=true,destructables=true;
    // A nuclear strike finishes hostile survivors through native death, including damage immunity/Avul.
    bool forceEnemyDeath=true;
    float radius=8000,visualRadius=3200,explosionScale=12,flameScale=7,smokeScale=14;
    float flameDelay=.8f,flameSeconds=7,smokeSeconds=18,flashSeconds=.65f,shakeDegrees=.6f,screenRadius=10000;
    // A bounded plume count controls density independently of size and time; sound stays on the CS master bus.
    int smokeEmitters=37;float explosionVolumePercent=100;
    std::string explosionSound="layered";
    std::string code="PUTINMOLODEC";
    float SmokeStart() const{return flameDelay+flameSeconds;}
    float FinishedAt() const{return std::max(8.f,SmokeStart()+smokeSeconds);}
};
