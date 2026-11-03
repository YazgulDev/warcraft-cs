#pragma once
// Surf uses existing gravity, air steering and speed cap; these preferences reload on F8.
struct SurfSettings {
    bool enabled=true;
    float maximumAngle=80,contactDistance=8;
};
