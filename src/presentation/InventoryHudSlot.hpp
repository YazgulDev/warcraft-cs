#pragma once
#include <string>

// Copied native item state; no handles/pointers escape into rendering.
struct InventoryHudSlot {
    int type=0,charges=0;
    std::string icon;
};
