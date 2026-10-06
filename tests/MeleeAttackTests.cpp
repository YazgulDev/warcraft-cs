#include "../src/MeleeAttack.hpp"
#include <cstdio>
#include <initializer_list>
#include <cstdlib>

static void Require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main() {
    // Verify the player-facing tradeoff and that contacts happen before recovery ends.
    for (int weapon : {WeaponSlots::Knife, WeaponSlots::Sword}) {
        auto slash = MeleeAttack::Select(weapon, false), thrust = MeleeAttack::Select(weapon, true);
        Require(thrust.damage > slash.damage && thrust.range < slash.range, "heavy thrust needs stronger damage and shorter reach");
        Require(thrust.recoverySeconds > slash.recoverySeconds, "heavy thrust must recover slower");
        Require(thrust.contactSeconds > slash.contactSeconds, "heavy thrust needs a distinct windup");
        Require(thrust.contactSeconds < thrust.recoverySeconds, "contact must precede recovery");
        Require(!slash.secondary && thrust.secondary, "input must select the correct animation and contact sound");
    }
    std::puts("Melee invariants passed: right-click damage, reach, windup, recovery and mode");
}
