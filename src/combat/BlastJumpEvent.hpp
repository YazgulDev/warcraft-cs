#pragma once
#include "../config/BlastJumpSettings.hpp"
#include <array>
#include <vector>

// Copied detonation data crosses into movement only after native damage callbacks finish.
struct BlastJumpEvent {
    std::array<float,3> position{};
    float radius=0;
    BlastJumpSettings settings;
    const char* label="blast";
};
using BlastJumpEvents=std::vector<BlastJumpEvent>;
