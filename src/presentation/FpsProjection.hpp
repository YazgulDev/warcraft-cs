#pragma once
#include <algorithm>
#include <cmath>

// Native cameras prepare projection outside the world batch callback.
class FpsProjection {
public:
    static constexpr float WorldFarClip = 5000.0f;
    static float NearPlane(bool fps, float nativeNear, float nativeFar) {
        // The FPS camera explicitly owns this far distance; native portraits use a separate camera.
        // Shortening its near plane keeps terrain visible below an eye only 96 units above the ground.
        return fps && std::abs(nativeFar-WorldFarClip)<0.01f ? std::min(nativeNear,8.0f) : nativeNear;
    }
};
