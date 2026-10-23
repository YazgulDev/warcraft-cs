#pragma once

// A percentage roll belongs to each confirmed hostile creature kill, never to individual pellets/hits.
struct WeaponDropSettings {
    bool enabled=true;
    float chancePercent=.5f,lifetimeSeconds=120,pickupRadius=160;
    unsigned maximumDrops=64;
};
