#include "GraphicsApi.hpp"
#include "BuyMenuView.hpp"
#include "../combat/UnitScaling.hpp"
#include "BuyMenuLayout.hpp"
#include <gl/GL.h>
#include <cstdio>

namespace {
void Rect(float x,float y,float w,float h,bool outline=false) {
    glBegin(outline ? GL_LINE_LOOP : GL_QUADS);
    glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
const char* MainLabel(int key) {
    switch (key) { case 1:return "PISTOLS";case 2:return "SHOTGUNS";case 3:return "SMGS";case 4:return "RIFLES";case 5:return "MACHINE GUNS";case 6:return "PRIMARY AMMO";
        case 7:return "SECONDARY AMMO";case 8:return "EQUIPMENT";case 9:return "HALF-LIFE";default:return ""; }
}
}
namespace BuyMenuView {
void Draw(const ShooterController& controller,int width,int height,HWND window,const Text& text,const Text& titleText,const Preview& preview) {
    if (!controller.Buying()) return;
    BuyMenuLayout box(width,height);auto page=controller.BuyPage();const auto& settings=controller.Settings();
    const char* title=page==BuyMenu::Page::Main ? "BUY MENU" : page==BuyMenu::Page::Rifles ? "BUY RIFLES (PRIMARY WEAPON)" :
        page==BuyMenu::Page::Pistols ? "BUY PISTOLS (SECONDARY WEAPON)" :
        page==BuyMenu::Page::Shotguns ? "BUY SHOTGUNS" : page==BuyMenu::Page::SMGs ? "BUY SMGS" :
        page==BuyMenu::Page::Machineguns ? "BUY MACHINE GUNS" : page==BuyMenu::Page::HalfLife ? "BUY HALF-LIFE" : "BUY EQUIPMENT";
    // Recreate the original translucent black/amber panel, sparse category keys and red hovered rows.
    glColor4f(0,0,0,.77f);Rect(box.x,box.y,box.width,box.height);
    glColor4f(1,.72f,.22f,.9f);glLineWidth(1);
    glBegin(GL_LINES);glVertex2f(box.x,box.y+box.height*.12f);glVertex2f(box.x+box.width,box.y+box.height*.12f);glEnd();
    // The category heading uses its own larger HUD font, like the original CS panel.
    titleText(box.listX,box.y+box.height*.078f,title);
    POINT cursor={};GetCursorPos(&cursor);ScreenToClient(window,&cursor);
    // Optional-content rows and pointer hitboxes consume the same validated resource availability.
    int hover=BuyMenuLayout::Key(cursor.x,cursor.y,width,height,page,&settings);
    auto keys=BuyMenuLayout::Keys(page,&settings);int previewSlot=-1;
    for (size_t i=0;i<keys.size();++i) {
        int key=keys[i];float rowY=box.listY+float(i)*box.row;
        auto item=BuyCatalog::Find(BuyMenu::Category(page),key);
        bool selected=hover==key;
        if (selected) { glColor4f(.95f,.025f,.005f,.94f);Rect(box.listX,rowY,box.listWidth,box.row*.63f); }
        glColor4f(1,.72f,.22f,.95f);Rect(box.listX,rowY,box.listWidth,box.row*.63f,true);
        char label[100];sprintf_s(label,"%d %s",key,item ? item->name : MainLabel(key));
        // Long labels use familiar short gun names in the list; full names remain in the detail panel.
        if (item) sprintf_s(label,"%d %s",key,controller.WeaponData(item->slot).name);
        text(box.listX+box.width*.009f,rowY+box.row*.43f,label);
        if (item && (previewSlot<0 || selected)) previewSlot=item->slot;
    }
    glColor4f(1,.72f,.22f,.95f);Rect(box.listX,box.cancelY,box.listWidth,box.row*.63f,true);
    if (hover==0) { glColor4f(.95f,.025f,.005f,.94f);Rect(box.listX,box.cancelY,box.listWidth,box.row*.63f); }
    glColor4f(1,.72f,.22f,1);text(box.listX+box.width*.009f,box.cancelY+box.row*.43f,page==BuyMenu::Page::Main ? "0 CANCEL" : "0 BACK");
    if (previewSlot>=0) {
        auto item=BuyCatalog::Slot(previewSlot);const auto& weapon=controller.WeaponData(previewSlot);
        Rect(box.detailX,box.previewY,box.detailWidth,box.previewHeight,true);
        preview(previewSlot,box.detailX,box.previewY,box.detailWidth,box.previewHeight);
        // Show actual configurable gameplay values rather than unrelated real-world ballistics.
        glColor4f(1,.72f,.22f,1);char value[128];float infoY=box.previewY+box.previewHeight+box.row*.55f;
        auto line=[&](const char* label,const char* contents) {
            text(box.detailX,infoY,label);text(box.detailX+box.detailWidth*.43f,infoY,contents);infoY+=box.row*.48f;
        };
        text(box.detailX,infoY,item->name);infoY+=box.row*.75f;
        sprintf_s(value,": %s",CurrencyDisplay::Format(settings.weaponPrice[previewSlot],settings.economy).c_str());line("PRICE",value);
        sprintf_s(value,": %s",item->caliber);line("CALIBER / TYPE",value);
        // Keep magazine/reserve in one row so the existing panel also fits small windows.
        sprintf_s(value,": %d / %d",weapon.magazine,settings.maxReserve[previewSlot]);
        if(WeaponSlots::Energy(previewSlot)){sprintf_s(value,": %d",weapon.magazine);line("ENERGY CAPACITY",value);}
        else if(WeaponSlots::Grenade(previewSlot)){sprintf_s(value,": %d",weapon.magazine);line("CARRY LIMIT",value);}
        else if (!WeaponSlots::Melee(previewSlot) && previewSlot!=WeaponSlots::C4) line("CLIP / RESERVE",value);
        sprintf_s(value,": %.0f",settings.ContactDamage(previewSlot,false,controller.Status().attackDamage));line("BASE DAMAGE",value);
        if (weapon.interval>0 && !WeaponSlots::Melee(previewSlot)) {
            // Display the cadence actually used by combat, including the optional native IAS multiplier.
            float interval=UnitScaling::FireInterval(weapon.interval,settings.scaleFireRateFromAttackSpeed,controller.Status().attackSpeed);
            sprintf_s(value,": %.0f RPM",60/interval);line("RATE OF FIRE",value);
        }
        if (!WeaponSlots::Melee(previewSlot)) { sprintf_s(value,": %s",CurrencyDisplay::Format(settings.ammoPrice[previewSlot],settings.economy).c_str());line("AMMO PACK",value); }
        line("INVENTORY",controller.OwnsWeapon(previewSlot) ? ": OWNED" : ": NOT OWNED");
    } else {
        glColor4f(1,.72f,.22f,1);
        text(box.detailX,box.listY+box.row*.45f,"SELECT A CATEGORY");
        // Prices and wallet use the same selected currency after startup or F8.
        text(box.detailX,box.listY+box.row*1.2f,settings.economy.currency==EconomySettings::Currency::Dollars ? "PAY IN DOLLARS" : "PAY IN GOLD");
        // Display the same reloaded bindings that dispatch these actions.
        auto refill=controller.Shortcut(ShortcutAction::Refill)+": FREE AMMO REFILL";
        auto all=controller.Shortcut(ShortcutAction::AllWeapons)+": GET ALL WEAPONS FREE";
        text(box.detailX,box.listY+box.row*2.1f,refill.c_str());
        text(box.detailX,box.listY+box.row*2.85f,all.c_str());
    }
    char money[160];sprintf_s(money,"%s   |   %s / ESC: CLOSE   |   %s: CURRENT AMMO",controller.MoneyText().c_str(),controller.Shortcut(ShortcutAction::Buy).c_str(),controller.Shortcut(ShortcutAction::BuyAmmo).c_str());
    glColor4f(1,.72f,.22f,1);text(box.listX,box.y+box.height*.95f,money);
    // Warcraft's pointer is hidden with its RTS HUD; retain a visible pointer for mouse purchases.
    glColor4f(1,1,1,1);glBegin(GL_TRIANGLES);
    glVertex2i(cursor.x,cursor.y);glVertex2i(cursor.x+3,cursor.y+18);glVertex2i(cursor.x+13,cursor.y+12);glEnd();
}
}
