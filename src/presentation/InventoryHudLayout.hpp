#pragma once
#include "SpellHudLayout.hpp"
#include <algorithm>

struct InventoryHudLayout {
    int x=0,y=0,side=0,gap=0;
    static InventoryHudLayout Calculate(int width,int height,float percent,bool spells) {
        auto row=SpellHudLayout::Calculate(width,height,percent);if(!row.side)return {};
        int total=4*row.side+3*row.gap,side=std::max(8,(total-5*row.gap)/6);
        // Leave room for spell shortcut labels between the two rows.
        int y=spells ? row.y-std::max(22,row.side/3+8)-side : row.y+row.side-side;
        return {row.x+total-6*side-5*row.gap,y,side,row.gap};
    }
};
