#include "InventoryHudView.hpp"
#include <cstdio>
namespace {
void Rect(float x,float y,float w,float h) {glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();}
}
void InventoryHudView::Draw(const UnitHudSnapshot& unit,const InventoryHudLayout& l,HudIconCache& icons,const InventoryControls& controls,const Text& text) {
    if(!unit.actor || !l.side || !unit.inventorySize)return;
    glPushAttrib(GL_ALL_ATTRIB_BITS);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    if(controls.Open()) {
        glDisable(GL_TEXTURE_2D);glColor4f(.025f,.025f,.035f,.94f);
        Rect(float(l.x-6),float(l.y-42),float(6*l.side+5*l.gap+12),float(l.side+48));
        glColor4f(1,.85f,.35f,1);text(float(l.x),float(l.y-25),controls.TargetSlot()>=0 ? "CLICK TARGET | RMB: CANCEL" : "CLICK: USE | DRAG OUT: DROP");
    }
    for(int i=0;i<6;++i) {
        float x=float(l.x+i*(l.side+l.gap)),y=float(l.y),side=float(l.side);
        glDisable(GL_TEXTURE_2D);glColor4f(.36f,.29f,.15f,.95f);Rect(x-2,y-2,side+4,side+4);
        if(controls.PressedSlot()==i || controls.TargetSlot()==i) {glColor4f(1,.8f,.2f,1);Rect(x-3,y-3,side+6,side+6);}
        glColor4f(.025f,.025f,.03f,.92f);Rect(x,y,side,side);
        const auto& item=unit.items[i];GLuint texture=i<unit.inventorySize ? icons.Texture(item.icon) : 0;
        if(texture) {
            glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);glColor4f(1,1,1,1);
            glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(x,y);glTexCoord2f(1,0);glVertex2f(x+side,y);
            glTexCoord2f(1,1);glVertex2f(x+side,y+side);glTexCoord2f(0,1);glVertex2f(x,y+side);glEnd();
        }
        glDisable(GL_TEXTURE_2D);glColor4f(1,.9f,.55f,1);
        if(controls.Numbers() || controls.Open()) {char key[4];sprintf_s(key,"%d",i+1);text(x+3,y-6,key);}
        // Charges follow the native item count, including consumables with a single remaining use.
        if(item.charges>0) {char count[24];sprintf_s(count,"%d",item.charges);text(x+3,y+side-4,count);}
        else if(item.type && !texture)text(x+3,y+side*.6f,"?");
        else if(i>=unit.inventorySize) {glColor4f(.14f,.14f,.16f,.7f);Rect(x,y,side,side);}
    }
    glPopAttrib();
}
void InventoryHudView::Cursor(HWND window,const UnitHudSnapshot& unit,const InventoryHudLayout& l,HudIconCache& icons,const InventoryControls& controls) {
    if(!controls.Open() || !window)return;
    POINT cursor={};GetCursorPos(&cursor);ScreenToClient(window,&cursor);glPushAttrib(GL_ALL_ATTRIB_BITS);
    // A held icon follows the free cursor; the source remains unchanged until the native drop is accepted.
    if(controls.Dragging() && controls.PressedSlot()>=0 && controls.PressedSlot()<6) {
        GLuint texture=icons.Texture(unit.items[controls.PressedSlot()].icon);
        if(texture) {
            glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);glColor4f(1,1,1,.8f);
            float x=float(cursor.x+8),y=float(cursor.y+8),side=float(l.side);
            glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(x,y);glTexCoord2f(1,0);glVertex2f(x+side,y);
            glTexCoord2f(1,1);glVertex2f(x+side,y+side);glTexCoord2f(0,1);glVertex2f(x,y+side);glEnd();
        }
    }
    glDisable(GL_TEXTURE_2D);glColor4f(1,1,1,1);glBegin(GL_TRIANGLES);
    glVertex2i(cursor.x,cursor.y);glVertex2i(cursor.x+3,cursor.y+18);glVertex2i(cursor.x+13,cursor.y+12);glEnd();glPopAttrib();
}
