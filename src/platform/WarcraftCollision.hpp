#pragma once
#include "WarcraftApi.hpp"
#include "../movement/MovementPhysics.hpp"
#include "MovementObstacles.hpp"

class WarcraftCollision {
public:
    void Configure(uintptr_t base) { base_=base;obstacles_.Configure(base); }
    void Reset() { obstacles_.Reset(); terrain_.clear(); modelPathing_=false; blockedLog_ = 0; }
    void Move(wc3::Handle unit, MovementPhysics& movement, float dt);
private:
    bool Clear(float x, float y, float oldX, float oldY, const MovementPhysics& movement,
        const std::vector<Bounds3>& obstacles,float* normal=nullptr) const;
    MovementSurface Terrain(float x,float y,const MovementPhysics& movement) const;
    MovementSurface Floor(float x, float y, const MovementPhysics& movement, const std::vector<Bounds3>& obstacles) const;
    void Ceiling(float x,float y,MovementPhysics& movement,const std::vector<Bounds3>& obstacles) const;
    void Blocked(const char* reason, float x, float y, float oldX, float oldY, float bottom, float terrain) const;
    MovementObstacles obstacles_;
    uintptr_t base_=0;
    float radius_=24;
    mutable DWORD blockedLog_ = 0;
    // Cache only this simulation frame; map height changes must be visible on the next movement update.
    mutable std::map<std::pair<float,float>,MovementSurface> terrain_;
    // Publish only a fully cleared model-adjacent path; the final native setter must preserve that checked XY.
    mutable bool modelPathing_=false;
};
