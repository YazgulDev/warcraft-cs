#include "GraphicsApi.hpp"
#include "WeaponSelectionView.hpp"
#include <gl/GL.h>
#include <cstdio>

namespace {
void Rect(float x,float y,float w,float h) {
    glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
}
void WeaponSelectionView::Draw(const ShooterController& controller,int width,int height,const Text& text,const Preview& preview) {
    if(!controller.SelectingWeapon())return;
    int pending=controller.WeaponChoice().Pending(),group=WeaponSelection::Group(pending);
    // Start below the existing four-line command hints, keeping the complete primary stack above the HUD.
    float x=width*.025f,y=height*.16f,column=width*.12f,header=height*.047f,row=height*.066f;
    auto available=controller.AvailableWeapons();
    for(int g=1;g<=WeaponSlots::SelectionGroups;++g) {
        float gx=x+(g-1)*column;
        glColor4f(0,0,0,.8f);Rect(gx,y,column-3,header);
        glColor4f(1,.72f,.22f,1);char label[80];
        auto key=controller.Shortcut(ShortcutAction(int(ShortcutAction::Weapon1)+g-1));
        sprintf_s(label,"%s %s",key.c_str(),WeaponSelection::Label(g));text(gx+5,y+header*.65f,label);
        if(g==group){glColor4f(1,.72f,.22f,1);Rect(gx,y+header-2,column-3,2);}
    }
    auto items=WeaponSelection::Items(group,available);float listX=x+(group-1)*column,listWidth=width*.225f;
    size_t selectedIndex=0;for(size_t i=0;i<items.size();++i)if(items[i]==pending)selectedIndex=i;
    size_t count=std::min(size_t(8),items.size()),first=std::min(selectedIndex>3 ? selectedIndex-3 : 0,items.size()-count);
    for(size_t i=first;i<first+count;++i) {
        int slot=items[i];float ry=y+header+3+float(i-first)*row;
        bool selected=slot==pending;
        glColor4f(selected ? .22f : 0,selected ? .15f : 0,0,.88f);Rect(listX,ry,listWidth,row-2);
        glColor4f(1,.72f,.22f,1);
        if(selected)Rect(listX,ry,3,row-2);
        // Reuse optional hand-free previews. Names/selection remain usable without any private art.
        preview(slot,listX+5,ry+2,listWidth*.42f,row-6);
        glColor4f(1,.72f,.22f,1);text(listX+listWidth*.44f,ry+row*.57f,controller.WeaponData(slot).name);
    }
    float footer=y+header+3+float(count)*row;
    glColor4f(0,0,0,.85f);Rect(listX,footer,listWidth,height*.065f);
    glColor4f(1,.72f,.22f,1);text(listX+6,footer+height*.025f,"LMB: SELECT | WHEEL: BROWSE");
    text(listX+6,footer+height*.05f,"RMB / 0 / ESC: CANCEL");
}
