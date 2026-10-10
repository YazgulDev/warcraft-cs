#pragma once
#include "WarcraftApi.hpp"
#include "../movement/MovementPhysics.hpp"
#include "MovementObstacles.hpp"

class WarcraftCollision {
public:
    void Configure(uintptr_t base) { obstacles_.Configure(base); }
    void Reset() { obstacles_.Reset(); }
    void Move(wc3::Handle unit, MovementPhysics& movement, float dt);
private:
    bool Clear(float x, float y, float oldX, float oldY, const MovementPhysics& movement,
        const std::vector<Bounds3>& obstacles) const;
    float Floor(float x, float y, const MovementPhysics& movement, const std::vector<Bounds3>& obstacles) const;
    MovementObstacles obstacles_;
};
