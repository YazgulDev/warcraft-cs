#pragma once

// F8 swaps the complete ray/headshot policy without changing weapon ownership or native defenses.
struct HitDetectionSettings {
    bool preciseModels=true,headshots=true;
    // Direct Tau/Gluon contacts use the same named-head rule as conventional firearms.
    bool energyHeadshots=true;
    float headshotBoneWeight=.35f;
    float headshotMultiplier=2;
};
