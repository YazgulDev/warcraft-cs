#include "GraphicsApi.hpp"
#include "SettingsMenuView.hpp"
#include "SettingsMenuLayout.hpp"
#include "HudRenderState.hpp"
#include <gl/GL.h>
#include <algorithm>

namespace {
void Rect(float x,float y,float w,float h){glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();}
}
void SettingsMenuView::Draw(const SettingsMenu& menu,int width,int height,int cursorX,int cursorY,const Text& text) {
    if(!menu.Open() || width<1 || height<1)return;
    // Both clean F9 and normal HUD routes use the same isolated flat pass, including Direct3D emulation.
    HudRenderState state;GLint mode=0;glGetIntegerv(GL_MATRIX_MODE,&mode);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    SettingsMenuLayout box(width,height);glColor4f(0,0,0,.7f);Rect(0,0,float(width),float(height));
    glColor4f(.025f,.035f,.045f,.98f);Rect(box.x,box.y,box.width,box.height);
    glColor4f(1,.72f,.22f,1);text(box.x+box.width*.04f,box.y+box.height*.07f,"WARCRAFT CS SETTINGS");
    glColor4f(.7f,.75f,.8f,1);text(box.x+box.width*.04f,box.y+box.height*.115f,"Game continues while this menu is open.");
    for(int row=0;row<SettingsMenu::Rows;++row) {
        float y=box.rowY+row*box.row;
        glColor4f(row==menu.Selected() ? .18f : .055f,row==menu.Selected() ? .14f : .07f,.065f,1);Rect(box.x+box.width*.025f,y,box.width*.95f,box.row*.88f);
        glColor4f(.95f,.95f,.9f,1);text(box.x+box.width*.04f,y+box.row*.58f,SettingsMenu::Label(row));
        glColor4f(1,.75f,.3f,1);text(box.minusX,y+box.row*.55f,"-");text(box.plusX,y+box.row*.55f,"+");
        std::string value=menu.Value(row);if(value.size()>26)value=value.substr(0,23)+"...";
        text(box.trackX,y+box.row*(SettingsMenu::Numeric(row) ? .48f : .58f),value.c_str());
        if(SettingsMenu::Numeric(row)) {
            glColor4f(.2f,.25f,.3f,1);Rect(box.trackX,y+box.row*.7f,box.trackWidth,3);
            glColor4f(1,.7f,.2f,1);Rect(box.trackX,y+box.row*.7f,box.trackWidth*std::clamp(menu.Fraction(row),0.f,1.f),3);
        }
    }
    glColor4f(.75f,.8f,.85f,1);text(box.x+box.width*.04f,box.y+box.height*.755f,SettingsMenu::Hint(menu.Selected()));
    glColor4f(menu.Error().empty() ? .8f : 1,.45f,.25f,1);
    const char* notice=!menu.Error().empty() ? menu.Error().c_str() : menu.Dirty() ? "Unsaved changes. Apply to save and use them." : "Changes apply after Apply & Save.";
    text(box.x+box.width*.04f,box.y+box.height*.80f,notice);
    const float starts[]={.04f,.36f,.68f};const char* labels[]={"APPLY & SAVE","CANCEL","RESET THESE"};
    for(int i=0;i<3;++i){glColor4f(.17f,.20f,.23f,1);Rect(box.x+box.width*starts[i],box.actionY,box.width*.26f,box.row*.85f);glColor4f(1,.75f,.3f,1);text(box.x+box.width*(starts[i]+.015f),box.actionY+box.row*.57f,labels[i]);}
    glColor4f(.7f,.75f,.8f,1);text(box.x+box.width*.04f,box.y+box.height*.965f,"ARROWS: EDIT   ENTER: SAVE   ESC: CANCEL");
    // Draw our own pointer in both backends; no global ShowCursor counter is borrowed from Warcraft.
    glColor4f(1,1,1,1);glBegin(GL_TRIANGLES);glVertex2i(cursorX,cursorY);glVertex2i(cursorX+3,cursorY+18);glVertex2i(cursorX+13,cursorY+12);glEnd();
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(mode);
}
