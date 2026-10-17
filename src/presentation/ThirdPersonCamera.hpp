#pragma once
#include "ThirdPersonPose.hpp"
#include "../config/ThirdPersonSettings.hpp"
#include "../combat/UnitHitboxes.hpp"
#include "../combat/DestructableHitboxes.hpp"

// Terrain/destructables shorten the boom; building proxies are optional, independent of physical movement.
class ThirdPersonCamera {
public:
    ThirdPersonPose Position(wc3::Handle actor,float z,float yaw,float pitch,const ThirdPersonSettings& settings,
        UnitHitboxes& units,DestructableHitboxes& destructables) const;
};
