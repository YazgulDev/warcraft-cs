#pragma once
#include "../platform/WarcraftApi.hpp"

// A read-only snapshot of the controlled Warcraft unit's current restrictions.
struct UnitStatus {
    bool incapacitated = false, immobilized = false, disarmed = false;
    bool contained = false, devoured = false, hidden = false;
    float speedScale = 1, nativeSpeed = 0, defaultSpeed = 0;
    int stunCount = 0;
    static UnitStatus Read(wc3::Handle unit, uintptr_t gameBase);
    const char* Label() const;
};
