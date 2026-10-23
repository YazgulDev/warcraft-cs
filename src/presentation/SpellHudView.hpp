#pragma once
#include "SpellHudSlot.hpp"
#include "SpellHudLayout.hpp"
#include <windows.h>
#include "GraphicsApi.hpp"
#include <functional>
#include "HudIconCache.hpp"

// Presentation owns icon textures; the simulation supplies live cooldown values and shortcut labels.
class SpellHudView {
public:
    void Reset(bool deleteObjects) {icons_.Reset(deleteObjects);}
    bool Valid() const {return icons_.Valid();}
    void Draw(const SpellHudSlots& slots,const std::array<std::string,4>& keys,const SpellHudLayout& layout,
        bool cooldowns,const std::function<void(float,float,const char*)>& text);
private:
    HudIconCache icons_;
};
