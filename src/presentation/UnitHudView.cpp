#include "GraphicsApi.hpp"
#include "UnitHudView.hpp"
#include "../platform/NativePortrait.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
void Rect(float x,float y,float w,float h) {
    glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
float Fraction(float value,float maximum) {
    return std::isfinite(value) && std::isfinite(maximum) && maximum>0 ? std::clamp(value/maximum,0.f,1.f) : 0;
}
}
void UnitHudView::Draw(const UnitHudSnapshot& unit,const UnitHudLayout& l,NativePortrait& portrait,const Text& text) {
    if(!unit.actor || !l.portrait)return;
    glPushAttrib(GL_ALL_ATTRIB_BITS);glDisable(GL_TEXTURE_2D);glColor4f(.50f,.40f,.20f,1);
    Rect(float(l.x-3),float(l.y-3),float(l.portrait+6),float(l.portrait+6));
    glColor4f(.025f,.025f,.035f,.96f);Rect(float(l.x),float(l.y),float(l.portrait),float(l.portrait));
    // Native portrait capture currently belongs to the OpenGL backend only.
    if(!GraphicsBackend::IsD3D9())portrait.Draw(l.x,l.y,l.portrait);glDisable(GL_TEXTURE_2D);
    float x=float(l.x+l.portrait+l.gap),barHeight=l.portrait*.26f;
    auto bar=[&](float y,float fraction,float r,float g,float b,const char* label) {
        glDisable(GL_TEXTURE_2D);glColor4f(.35f,.29f,.16f,1);Rect(x-2,y-2,float(l.bars+4),barHeight+4);
        glColor4f(.015f,.02f,.025f,.94f);Rect(x,y,float(l.bars),barHeight);
        glColor4f(r,g,b,.95f);Rect(x,y,l.bars*fraction,barHeight);
        glColor4f(1,1,1,1);text(x+5,y+barHeight*.77f,label);
    };
    char label[80];sprintf_s(label,"HP %.0f / %.0f",unit.health,unit.maxHealth);
    bar(float(l.y),Fraction(unit.health,unit.maxHealth),.12f,.62f,.18f,label);
    sprintf_s(label,"MANA %.0f / %.0f",unit.mana,unit.maxMana);
    bar(l.y+l.portrait*.36f,Fraction(unit.mana,unit.maxMana),.12f,.32f,.83f,label);
    // Capped heroes have no next threshold; ordinary units never receive an invented XP bar.
    if(unit.hero) {
        float xp=0;
        if(unit.experienceKnown) {
            bool capped=unit.nextExperience==unit.levelExperience;
            xp=capped ? 1.f : Fraction(float(unit.experience-unit.levelExperience),float(unit.nextExperience-unit.levelExperience));
            if(capped)sprintf_s(label,"LV %d  MAX",unit.level);
            else sprintf_s(label,"LV %d  EXP %d / %d",unit.level,std::max(0,unit.experience-unit.levelExperience),unit.nextExperience-unit.levelExperience);
        } else sprintf_s(label,"LV %d  EXP %d",unit.level,unit.experience);
        bar(l.y+l.portrait*.72f,xp,.56f,.20f,.75f,label);
    }
    glPopAttrib();
}
