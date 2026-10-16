#pragma once
#include "SpellHudLayout.hpp"
#include <algorithm>

struct InventoryHudLayout {
    int x=0,y=0,side=0,gap=0;
    int Slot(int px,int py) const {
        if(side<=0 || py<y || py>=y+side || px<x)return -1;
        int slot=(px-x)/(side+gap);return slot<6 && (px-x)%(side+gap)<side ? slot : -1;
    }
    int PointerSlot(int px,int py) const {
        int slot=Slot(px,py);if(slot>=0)return slot;
        // Gaps and the panel heading cancel a drag; only leaving the panel deliberately drops an item.
        bool panel=side>0 && px>=x-6 && px<x+6*side+5*gap+6 && py>=y-42 && py<y+side+6;
        return panel ? -2 : -1;
    }
    static InventoryHudLayout Calculate(int width,int height,float percent,bool spells,bool interactive=false) {
        auto row=SpellHudLayout::Calculate(width,height,percent);if(!row.side)return {};
        int total=4*row.side+3*row.gap,side=std::max(8,(total-5*row.gap)/6);
        // Mouse mode enlarges the same six slots while retaining the spell row beneath them.
        if(interactive)side=std::min(row.side,(width-2*std::max(12,int(height*.022f))-5*row.gap)/6);
        // Leave room for spell shortcut labels between the two rows.
        int y=spells ? row.y-std::max(22,row.side/3+8)-side : row.y+row.side-side;
        return {row.x+total-6*side-5*row.gap,y,side,row.gap};
    }
};
