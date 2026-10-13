#pragma once
#include "../config/ThirdPersonSettings.hpp"
#include <algorithm>

// Retain precision-wheel fractions without wrapping camera distance like weapon slots.
class CameraWheel {
public:
    void Add(int delta) {delta_+=delta;}
    void Reset() {delta_=0;}
    float Take(float distance,const ThirdPersonSettings& settings) {
        constexpr int notch=120;
        auto steps=delta_/notch;delta_%=notch;
        // Scroll up pulls the requested boom inward; native collision may shorten it further.
        return std::clamp(distance-float(steps)*settings.zoomStep,settings.minimumDistance,settings.maximumDistance);
    }
private:
    long long delta_=0;
};
