#pragma once
#include <string>

// U enables a session policy; resource spending still goes through Warcraft's normal orders.
struct ArmyAISettings {
    bool enabled=true;
    // U manages the base by default; following the controlled hero requires an explicit opt-in.
    bool followActor=false;
    // Feedback explains campaign restrictions; bounded site retries do not disturb existing unit orders.
    bool showStatus=true;
    unsigned placementAttempts=12;
    // Undead gold workers cannot gather wood; reserve a few unassigned ghouls for native harvesting.
    int lumberWorkers=2;
    unsigned intervalMs=1500;
    unsigned productionRetrySeconds=180;
    int workers=6,soldiers=24,defenders=3,supplyBuildings=6;
    float baseRadius=900,supportDistance=350,buildRadius=450,resourceRadius=1600;
    // Empty rawcodes choose the worker's stock race. Custom maps can supply one coherent build plan.
    std::string worker,hall,supply,barracks,soldier;
};
