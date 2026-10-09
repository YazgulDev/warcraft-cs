#pragma once
#include "WarcraftApi.hpp"
#include "../movement/MovementPhysics.hpp"

class WarcraftCollision {
public:
    void Move(wc3::Handle unit, MovementPhysics& movement, float dt) const;
private:
    bool Clear(float x, float y, float oldFloor) const;
};
