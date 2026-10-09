#pragma once
#include "WeaponSlots.hpp"

// Separate attacks share recovery, but a right-click thrust trades reach and speed for damage.
struct MeleeAttack {
    float damage, range, contactSeconds, recoverySeconds;
    bool secondary;
    static MeleeAttack Select(int weapon, bool secondary) {
        if (weapon == WeaponSlots::Sword)
            return secondary ? MeleeAttack{240, 200, .65f, 2.2f, true} : MeleeAttack{120, 230, .43f, 1.84f, false};
        return secondary ? MeleeAttack{65, 105, .25f, 1.05f, true} : MeleeAttack{40, 150, .12f, .5f, false};
    }
};
