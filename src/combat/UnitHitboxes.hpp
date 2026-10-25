#pragma once
#include "../platform/WarcraftApi.hpp"
#include "../geometry/RayBounds.hpp"
#include <map>
#include "../geometry/AnimatedHitModel.hpp"

// Reads the current map's own model extents; handles and native pointers never survive map changes.
class UnitHitboxes {
public:
    void Configure(uintptr_t base) { base_ = base; }
    void Reset() { models_.clear();shotModels_.clear(); }
    bool Intersect(wc3::Handle unit, const float* origin, const float* direction, float limit,
        float& entry, float padding = 0);
    // Firearms use posed surfaces; spell/TPS/melee callers retain their existing volume contract.
    bool IntersectShot(wc3::Handle unit,const float* origin,const float* direction,float limit,float& entry,bool& head);
private:
    uintptr_t LocalRay(wc3::Handle unit,const float* origin,const float* direction,float* localOrigin,float* localDirection,float& scale) const;
    bool Pose(uintptr_t model,const AnimatedHitModel& mesh,int& sequence,float& time) const;
    Bounds3 Load(int type, bool building);
    uintptr_t base_ = 0;
    std::map<int, Bounds3> models_;
    std::map<int,AnimatedHitModel> shotModels_;
};
