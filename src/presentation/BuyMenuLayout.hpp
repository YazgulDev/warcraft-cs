#pragma once
#include "../economy/BuyMenu.hpp"
#include "../config/GameplaySettings.hpp"
#include <algorithm>
#include <vector>

// Normalized CS panel geometry shares sparse category numbers with mouse hit-testing.
struct BuyMenuLayout {
    float x,y,width,height,row,listX,listY,listWidth,detailX,detailWidth,previewY,previewHeight,cancelY,paymentY;
    BuyMenuLayout(int w,int h) {
        width=w*.72f;height=h*.86f;x=(w-width)*.5f;y=h*.06f;
        row=height*.064f;listX=x+width*.053f;listY=y+height*.23f;listWidth=width*.31f; // Nine categories fit above Back/Cancel.
        detailX=x+width*.405f;detailWidth=width*.54f;previewY=listY;previewHeight=height*.19f;
        cancelY=y+height*.83f;
        paymentY=y+height*.15f;
    }
    static std::vector<int> Keys(BuyMenu::Page page,const GameplaySettings* settings=nullptr) {
        if (page==BuyMenu::Page::Closed) return {};
        if (page==BuyMenu::Page::Main) return {1,2,3,4,5,6,7,8,9};
        // Rendering and pointer hitboxes omit absent optional content using the same availability mask.
        std::vector<int> keys;
        for (const auto& item : BuyCatalog::Entries) if (item.category==BuyMenu::Category(page) && (!settings || settings->WeaponEnabled(item.slot))) keys.push_back(item.key);
        std::sort(keys.begin(),keys.end());return keys;
    }
    static int Key(int px,int py,int w,int h,BuyMenu::Page page,const GameplaySettings* settings=nullptr) {
        if (page==BuyMenu::Page::Closed) return -1;
        BuyMenuLayout box(w,h);
        // Match the visible payment buttons before testing the category list below them.
        if(py>=box.paymentY && py<box.paymentY+box.row*.7f) {
            if(px>=box.listX && px<=box.listX+box.listWidth)return BuyMenu::PayDollars;
            if(px>=box.detailX && px<=box.detailX+box.detailWidth)return BuyMenu::PayGold;
        }
        if (px<box.listX || px>box.listX+box.listWidth) return -1;
        if (py>=box.cancelY && py<box.cancelY+box.row*.63f) return 0;
        const auto keys=Keys(page,settings);
        if (py<box.listY) return -1;
        int index=int((py-box.listY)/box.row);
        if (index<0 || index>=int(keys.size()) || py>=box.listY+index*box.row+box.row*.63f) return -1;
        return keys[index];
    }
};
