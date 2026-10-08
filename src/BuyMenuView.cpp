#include "BuyMenuView.hpp"
#include "BuyMenuLayout.hpp"
#include <gl/GL.h>
#include <vector>
#include <string>
#include <cstdio>

namespace BuyMenuView {
void Draw(const ShooterController& controller,int width,int height,HWND window,const Text& text) {
    if (!controller.Buying()) return;
    BuyMenuLayout box(width,height);std::vector<std::string> rows;
    auto page=controller.BuyPage();const auto& settings=controller.Settings();
    auto weapon=[&](int key,int slot) {
        char row[96];sprintf_s(row,"%d  %s   %d GOLD%s",key,ShooterController::weapons[slot].name,settings.weaponPrice[slot],
            controller.OwnsWeapon(slot) && slot!=WeaponSlots::C4 ? "  [OWNED]" : "");rows.push_back(row);
    };
    const char* title="BUY EQUIPMENT";
    if (page==BuyMenu::Page::Main) {
        rows={"1  PISTOLS","2  RIFLES","3  MELEE"};weapon(4,WeaponSlots::C4);
        char row[96];int slot=controller.WeaponIndex();
        sprintf_s(row,"5  CURRENT AMMO   %d GOLD",settings.ammoPrice[slot]);rows.push_back(row);
        rows.push_back("6  AMMO: ONE PACK FOR EACH OWNED WEAPON");
    } else if (page==BuyMenu::Page::Pistols) { title="BUY PISTOLS";weapon(1,2); }
    else if (page==BuyMenu::Page::Rifles) { title="BUY RIFLES";weapon(1,0);weapon(2,1);weapon(3,3); }
    else { title="BUY MELEE";weapon(1,WeaponSlots::Knife);weapon(2,WeaponSlots::Sword); }
    rows.push_back(page==BuyMenu::Page::Main ? "0  CLOSE" : "0  BACK");
    // The game continues behind the menu; the panel never changes map pause state or the native UI.
    glColor4f(.025f,.035f,.04f,.94f);glBegin(GL_QUADS);
    glVertex2f(box.x,box.y);glVertex2f(box.x+box.width,box.y);
    glVertex2f(box.x+box.width,box.y+box.row*(3+rows.size()));glVertex2f(box.x,box.y+box.row*(3+rows.size()));glEnd();
    glColor4f(1,.85f,.3f,1);text(box.x+18,box.y+box.row*.8f,title);
    char money[96];sprintf_s(money,"GOLD %d | B / ESC: CLOSE | .: CURRENT AMMO",controller.Gold());
    text(box.x+18,box.y+box.row*1.7f,money);
    POINT cursor={};GetCursorPos(&cursor);ScreenToClient(window,&cursor);
    int hover=BuyMenuLayout::Key(cursor.x,cursor.y,width,height,page);
    for (size_t row=0;row<rows.size();++row) {
        int key=row==rows.size()-1 ? 0 : int(row)+1;
        if (hover==key) glColor4f(1,1,.65f,1);else glColor4f(.8f,.88f,.88f,1);
        text(box.x+18,box.y+box.row*(3+float(row))-12,rows[row].c_str());
    }
    // Native HUD hiding also hides Warcraft's pointer, so show a lightweight menu cursor ourselves.
    glColor4f(1,1,1,1);glBegin(GL_TRIANGLES);
    glVertex2i(cursor.x,cursor.y);glVertex2i(cursor.x+3,cursor.y+18);glVertex2i(cursor.x+13,cursor.y+12);glEnd();
}
}
