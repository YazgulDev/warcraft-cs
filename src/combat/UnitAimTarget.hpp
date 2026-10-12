#pragma once
#include <cstdint>

// One read-only attack contact serves combat and the third-person target indicator.
struct UnitAimTarget {
    uint32_t handle=0;
    bool destructable=false,allied=false;
    float distance=0,position[3]={};
};
