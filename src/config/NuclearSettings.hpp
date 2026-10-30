#pragma once
#include <string>

// A code-only consumable has its own inventory, never free refill, random loot or the buy catalog.
struct NuclearSettings {
    bool enabled=true,repeatCodes=false,buildings=false,rewards=true;
    float delay=3,radius=0,targetDistance=800,flashSeconds=.65f,shakeDegrees=.6f,effectSeconds=8;
    std::string code="PUTINMOLODEC";
};
