#pragma once
#include "UnitHudSnapshot.hpp"
#include "UnitHudLayout.hpp"
#include "InventoryHudLayout.hpp"
#include "HudIconCache.hpp"
#include <functional>
class NativePortrait;

class UnitHudView {
public:
    using Text=std::function<void(float,float,const char*)>;
    static void Draw(const UnitHudSnapshot& unit,const UnitHudLayout& layout,NativePortrait& portrait,const Text& text);
    static void Inventory(const UnitHudSnapshot& unit,const InventoryHudLayout& layout,HudIconCache& icons,const Text& text);
};
