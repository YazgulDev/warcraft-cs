#pragma once
#include <cstdint>
#include <array>

// One read-only attack contact serves combat and the third-person target indicator.
struct UnitAimTarget {
    uint32_t handle=0;
    bool destructable=false,allied=false;
    float distance=0,position[3]={};
    bool enemy=false;
    // Ground-conforming ring vertices are sampled on the game thread, never from rendering callbacks.
    std::array<std::array<float,3>,32> circle{};
};
