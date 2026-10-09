#pragma once
#include <array>
#include <cstdint>

// GoldSrc ran1/RandomLong behavior, referenced from ReHLDS (MIT; see NOTICE/licenses).
// Shuffled Park-Miller and rejection sampling use state isolated from Warcraft's gameplay RNG.
class GoldSrcRandom {
public:
    void Reset(int32_t seed) { state_ = seed > 0 ? seed : 1; shuffle_ = 0; }
    unsigned Uniform(unsigned maximum) {
        unsigned range = maximum + 1, sample;
        const unsigned ceiling = 0x7fffffffu - (0x80000000u % range);
        do { sample = Next(); } while (sample > ceiling);
        return sample % range;
    }
private:
    int32_t Advance() {
        // Schrage's decomposition preserves the original 31-bit recurrence without signed overflow.
        int32_t quotient = state_ / 127773;
        state_ = 16807 * (state_ - quotient * 127773) - 2836 * quotient;
        if (state_ < 0) state_ += 2147483647;
        return state_;
    }
    unsigned Next() {
        if (!shuffle_) {
            for (int i = 39; i >= 0; --i) { Advance(); if (i < 32) table_[i] = state_; }
            shuffle_ = table_[0];
        }
        Advance();
        int index = shuffle_ / 67108864;
        shuffle_ = table_[index]; table_[index] = state_;
        return unsigned(shuffle_);
    }
    int32_t state_ = 0xC516, shuffle_ = 0;
    std::array<int32_t, 32> table_{};
};
