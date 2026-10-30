#pragma once
#include "../config/SettingsMenu.hpp"

struct SettingsMenuLayout {
    float x,y,width,height,rowY,row,trackX,trackWidth,minusX,plusX,buttonWidth,actionY;
    enum class HitKind {None,Row,Minus,Plus,Slide,Apply,Cancel,Defaults};
    struct Hit {HitKind kind=HitKind::None;int row=-1;float fraction=0;};
    SettingsMenuLayout(int w,int h) {
        width=w*.8f;height=h*.88f;x=(w-width)*.5f;y=(h-height)*.5f;rowY=y+height*.15f;row=height*.072f;
        minusX=x+width*.50f;plusX=x+width*.92f;buttonWidth=width*.035f;trackX=x+width*.55f;trackWidth=width*.34f;actionY=y+height*.85f;
    }
    float Fraction(float px) const {return (px-trackX)/trackWidth;}
    Hit Test(int px,int py) const {
        if(px<x || px>x+width)return {};
        if(py>=actionY && py<actionY+row*.85f) {
            if(px>=x+width*.04f && px<x+width*.3f)return {HitKind::Apply};
            if(px>=x+width*.36f && px<x+width*.62f)return {HitKind::Cancel};
            if(px>=x+width*.68f && px<x+width*.96f)return {HitKind::Defaults};
        }
        int index=int((py-rowY)/row);
        if(py<rowY || index>=SettingsMenu::Rows || py>=rowY+(index+.88f)*row)return {};
        if(px>=minusX && px<minusX+buttonWidth)return {HitKind::Minus,index};
        if(px>=plusX && px<plusX+buttonWidth)return {HitKind::Plus,index};
        if(SettingsMenu::Numeric(index) && px>=trackX && px<=trackX+trackWidth)return {HitKind::Slide,index,Fraction(float(px))};
        return {HitKind::Row,index};
    }
};
