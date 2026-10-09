#pragma once
#include "../combat/WeaponSlots.hpp"

// Retain high-resolution wheel fractions until a full Windows notch selects a weapon.
class WeaponWheel {
public:
    void Add(int delta) { delta_ += delta; }
    void Reset() { delta_ = 0; }
    int Take(int current) {
        constexpr int notch = 120;
        int steps = static_cast<int>((delta_ / notch) % WeaponSlots::Count);
        delta_ %= notch;
        // Scroll up selects the previous slot; scroll down selects the next, including wraparound.
        return (current - steps + WeaponSlots::Count) % WeaponSlots::Count;
    }
private:
    long long delta_ = 0;
};
