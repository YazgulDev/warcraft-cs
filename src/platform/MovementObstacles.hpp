#pragma once
#include "WarcraftApi.hpp"
#include "../geometry/ModelTransform.hpp"
#include "../geometry/RayBounds.hpp"
#include <map>

// Native spatial queries feed conservative 3D obstacles; their model cache belongs to the active map.
class MovementObstacles {
public:
    void Configure(uintptr_t base) { base_ = base; }
    void Reset() { models_.clear(); paths_.clear(); }
    std::vector<Bounds3> Snapshot(wc3::Handle actor, float x, float y, float radius);
private:
    bool Bounds(const std::string& path, uintptr_t object, Bounds3& world);
    uintptr_t base_ = 0;
    std::map<std::string, Bounds3> models_;
    std::map<std::pair<int,bool>, std::string> paths_;
};
