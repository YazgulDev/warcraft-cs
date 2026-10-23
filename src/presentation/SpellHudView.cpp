#include "GraphicsApi.hpp"
#include "SpellHudView.hpp"
#include "SpellCooldownWedge.hpp"
#include <cstdio>
#include <cmath>

void SpellHudView::Draw(const SpellHudSlots& slots,const std::array<std::string,4>& keys,const SpellHudLayout& layout,
    bool cooldowns,const std::function<void(float,float,const char*)>& text) {
    if(layout.side<=0)return;glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);glDisable(GL_ALPHA_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    auto quad=[](float x,float y,float side) {glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+side,y);glVertex2f(x+side,y+side);glVertex2f(x,y+side);glEnd();};
    for(int i=0;i<4;++i) {
        const auto& slot=slots[i];float x=float(layout.x+i*(layout.side+layout.gap)),y=float(layout.y),side=float(layout.side);
        glDisable(GL_TEXTURE_2D);glColor4f(.02f,.02f,.02f,.9f);quad(x-2,y-2,side+4);
        GLuint icon=icons_.Texture(slot.icon);
        if(icon) {
            glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,icon);
            // Disabled/unlearned skills dim; mana shortage uses Warcraft's blue visual cue.
            float light=slot.level && !slot.disabled ? 1.f : .35f;
            glColor4f(slot.noMana ? light*.4f : light,slot.noMana ? light*.4f : light,light,1);
            glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(x,y);glTexCoord2f(1,0);glVertex2f(x+side,y);
            glTexCoord2f(1,1);glVertex2f(x+side,y+side);glTexCoord2f(0,1);glVertex2f(x,y+side);glEnd();
        }
        glDisable(GL_TEXTURE_2D);
        if(cooldowns && slot.level && !slot.passive && slot.cooldownKnown && slot.cooldown>.05f) {
            // Only a live native countdown darkens an icon; a rejected order cannot start a fake cooldown.
            glColor4f(0,0,0,.65f);
            auto wedge=SpellCooldownWedge(slot.cooldown,slot.cooldownTotal);
            if(wedge.empty())quad(x,y,side);
            else {glBegin(GL_TRIANGLE_FAN);for(const auto& p:wedge)glVertex2f(x+p[0]*side,y+p[1]*side);glEnd();}
        }
        glColor4f(1,.88f,.35f,1);text(x+3,y-7,keys[i].c_str());
        char label[32];
        if(!slot.ability)sprintf_s(label,"EMPTY");
        else if(!slot.level)sprintf_s(label,"LV0");
        else if(slot.passive)sprintf_s(label,"PASS");
        else if(slot.disabled)sprintf_s(label,"DISABLED");
        else if(slot.noMana && (!slot.cooldownKnown || slot.cooldown<=.05f))sprintf_s(label,"NO MANA");
        else if(!cooldowns)continue;
        // Charge spells may cast repeatedly until empty; their map timer must never be invented locally.
        else if(slot.charges>=0)sprintf_s(label,"%d CHG",slot.charges);
        else if(!slot.cooldownKnown)sprintf_s(label,"?");
        else if(slot.cooldown>=10)sprintf_s(label,"%.0fs",std::ceil(slot.cooldown));
        else if(slot.cooldown>.05f)sprintf_s(label,"%.1fs",slot.cooldown);
        else sprintf_s(label,"READY");
        text(x+3,y+side+19,label);
    }
    glPopAttrib();
}
