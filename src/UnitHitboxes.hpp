#pragma once
#include "WarcraftApi.hpp"
#include "RayBounds.hpp"
#include <map>

// Reads the current map's own model extents; handles and native pointers never survive map changes.
class UnitHitboxes {
public:
    void Configure(uintptr_t base) { base_ = base; }
    void Reset() { models_.clear(); }
    bool Intersect(wc3::Handle unit, const float* origin, const float* direction, float limit,
        float& entry, float padding = 0);
private:
    Bounds3 Load(int type, bool building);
    uintptr_t base_ = 0;
    std::map<int, Bounds3> models_;
};
