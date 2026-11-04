#pragma once
// Surf uses existing gravity, air steering and speed cap; these preferences reload on F8.
struct SurfSettings {
    bool enabled=true;
    bool modelContacts=true; // Verified stationary model faces use the same motor as terrain surf.
    float accelerationMultiplier=2; // Contact-only air steering; 1 retains the original surf response.
    float maximumAngle=80,contactDistance=8;
};
