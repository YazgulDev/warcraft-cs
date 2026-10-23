#pragma once
#include <array>
#include <cstdint>

// A read-only contact snapshot cannot cause another damage tick while rendering.
struct EnergyBeam {
    std::array<float,3> start{},end{};
    uint32_t tick=0;
    float seconds=.15f,width=3;
    bool gluon=false;
};
