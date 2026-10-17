#pragma once
#include "UnitHudSnapshot.hpp"
#include "InventoryHudLayout.hpp"
#include "HudIconCache.hpp"
#include "../inventory/InventoryControls.hpp"
#include <functional>

class InventoryHudView {
public:
    using Text=std::function<void(float,float,const char*)>;
    static void Draw(const UnitHudSnapshot& unit,const InventoryHudLayout& layout,HudIconCache& icons,const InventoryControls& controls,const Text& text);
    static void Cursor(HWND window,const UnitHudSnapshot& unit,const InventoryHudLayout& layout,HudIconCache& icons,const InventoryControls& controls);
};
