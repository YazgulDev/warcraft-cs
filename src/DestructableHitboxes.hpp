#pragma once
#include "WarcraftApi.hpp"
#include "RayBounds.hpp"
#include <map>

// Gates, trees and other destructable widgets are independent of Warcraft's unit enumeration.
class DestructableHitboxes {
public:
    void Configure(uintptr_t base) { base_ = base; }
    void Reset() { models_.clear(); }
    bool Intersect(wc3::Handle target, const float* origin, const float* direction, float limit, float& entry);
private:
    uintptr_t base_ = 0;
    std::map<int, Bounds3> models_;
};
