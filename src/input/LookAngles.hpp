#pragma once
#include <algorithm>
#include <cmath>

namespace LookAngles {
// Bound accumulated turns without introducing a discontinuity in direction at 0/360 degrees.
inline float Normalize(float degrees) {
    float result=std::fmod(degrees,360.0f);
    return result<0 ? result+360.0f : result;
}
inline void Apply(float& yaw,float& pitch,float dx,float dy,float sensitivity) {
    // Fast swipes are real input, not cursor-warp errors: never discard them by a pixel threshold.
    yaw=Normalize(yaw-dx*sensitivity);
    pitch=std::clamp(pitch-dy*sensitivity,-65.0f,65.0f);
}
}
