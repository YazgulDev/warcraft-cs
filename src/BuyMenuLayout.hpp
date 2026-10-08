#pragma once
#include "BuyMenu.hpp"
#include <algorithm>

// Shared geometry keeps mouse hit-testing aligned with the rendered rows at every window size.
struct BuyMenuLayout {
    float x,y,width,row;
    BuyMenuLayout(int w,int h) {
        float scale=std::clamp(std::min(w/900.0f,h/650.0f),.6f,1.3f);
        width=660*scale;x=(w-width)/2;y=h*.18f;row=44*scale;
    }
    static int Key(int x,int y,int width,int height,BuyMenu::Page page) {
        BuyMenuLayout box(width,height);
        int rows=page==BuyMenu::Page::Main ? 7 : page==BuyMenu::Page::Rifles ? 4 : page==BuyMenu::Page::Melee ? 3 : 2;
        if (x<box.x || x>box.x+box.width || y<box.y+box.row*2 || y>=box.y+box.row*(2+rows)) return -1;
        int index=int((y-box.y)/box.row)-2;
        return index==rows-1 ? 0 : index+1;
    }
};
