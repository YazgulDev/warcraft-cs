#pragma once
#include "WarcraftApi.hpp"
#include "../presentation/UnitHudSnapshot.hpp"

// Read native map-resolved hero thresholds and item button art without copying game tables/assets.
class NativeUnitHud {
public:
    void Configure(uintptr_t base);
    UnitHudSnapshot Read(wc3::Handle actor,bool inventory) const;
private:
    uintptr_t base_=0;
    bool experienceVerified_=false,artVerified_=false;
};
