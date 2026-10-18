#pragma once
#include "../combat/WeaponSlots.hpp"

// Retain high-resolution wheel fractions until a full Windows notch selects a weapon.
class WeaponWheel {
public:
    void Add(int delta) { delta_ += delta; }
    void Reset() { delta_ = 0; }
    long long TakeSteps() {
        // Preserve signed detents before wrapping any inventory: a large batch cannot reverse direction.
        long long steps=-(delta_/120);delta_%=120;return steps;
    }
    int Take(int current) {
        int steps=int(TakeSteps()%WeaponSlots::Count);
        // Scroll up selects the previous slot; scroll down selects the next, including wraparound.
        return (current + steps + WeaponSlots::Count) % WeaponSlots::Count;
    }
private:
    long long delta_ = 0;
};
