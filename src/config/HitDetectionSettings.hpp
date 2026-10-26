#pragma once

// F8 swaps the complete ray/headshot policy without changing weapon ownership or native defenses.
struct HitDetectionSettings {
    bool preciseModels=true,headshots=true;
    float headshotMultiplier=2;
};
