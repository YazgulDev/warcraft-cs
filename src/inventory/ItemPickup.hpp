#pragma once
#include "../platform/WarcraftApi.hpp"
#include "ItemPickupResult.hpp"

class ItemPickup {
public:
    wc3::Handle Nearest(wc3::Handle unit,float radius) const;
    ItemPickupResult Take(wc3::Handle unit,float radius) const;
};
