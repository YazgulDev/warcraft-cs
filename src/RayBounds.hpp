#pragma once
#include <algorithm>
#include <cmath>

struct Bounds3 {
    float minimum[3] = {-45, -45, 0};
    float maximum[3] = {45, 45, 145};
};

// Slab intersection finds the first surface, including vertical shots and origins inside a volume.
inline bool IntersectBounds(const Bounds3& bounds, const float* origin, const float* direction,
    float limit, float& entry) {
    float nearT = 0, farT = limit;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(direction[axis]) < 0.00001f) {
            if (origin[axis] < bounds.minimum[axis] || origin[axis] > bounds.maximum[axis]) return false;
        } else {
            float a = (bounds.minimum[axis] - origin[axis]) / direction[axis];
            float b = (bounds.maximum[axis] - origin[axis]) / direction[axis];
            if (a > b) std::swap(a, b);
            nearT = std::max(nearT, a); farT = std::min(farT, b);
            if (nearT > farT) return false;
        }
    }
    entry = nearT;
    return farT >= 0 && nearT < limit;
}
