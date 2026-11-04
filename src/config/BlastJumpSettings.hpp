#pragma once

// Explosions snapshot movement tuning on launch/plant; velocities use GoldSrc units/s.
struct BlastJumpSettings {
    bool enabled=true;
    float speed=650,maximumUpSpeed=1500;
};
