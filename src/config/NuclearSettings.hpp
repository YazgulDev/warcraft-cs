#pragma once
#include <string>
#include <algorithm>

// The code-only charge uses shared RPG flight and a bounded impact, never purchases/refills/random loot.
struct NuclearSettings {
    bool enabled=true,repeatCodes=false,buildings=false,rewards=true;
    bool neutralBuildings=true,neutralUnits=true,destructables=true;
    float radius=1600,visualRadius=1200,explosionScale=8,flameScale=5,smokeScale=6;
    float flameDelay=.8f,flameSeconds=7,smokeSeconds=8,flashSeconds=.65f,shakeDegrees=.6f,screenRadius=2400;
    std::string code="PUTINMOLODEC";
    float SmokeStart() const{return flameDelay+flameSeconds;}
    float FinishedAt() const{return std::max(8.f,SmokeStart()+smokeSeconds);}
};
