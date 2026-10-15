#pragma once
#include "WarcraftApi.hpp"
#include "../movement/MovementPhysics.hpp"
#include "MovementObstacles.hpp"

class WarcraftCollision {
public:
    void Configure(uintptr_t base) { base_=base;obstacles_.Configure(base); }
    void Reset() { obstacles_.Reset(); blockedLog_ = 0; }
    void Move(wc3::Handle unit, MovementPhysics& movement, float dt);
private:
    bool Clear(float x, float y, float oldX, float oldY, const MovementPhysics& movement,
        const std::vector<Bounds3>& obstacles) const;
    float Floor(float x, float y, const MovementPhysics& movement, const std::vector<Bounds3>& obstacles) const;
    void Blocked(const char* reason, float x, float y, float oldX, float oldY, float bottom, float terrain) const;
    MovementObstacles obstacles_;
    uintptr_t base_=0;
    float radius_=24;
    mutable DWORD blockedLog_ = 0;
};
