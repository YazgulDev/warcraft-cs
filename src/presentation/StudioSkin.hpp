#pragma once
#include <array>
#include <cmath>
#include <algorithm>

namespace StudioSkin {
inline std::array<float,3> Normal(const float* bone,float x,float y,float z,bool held) {
    std::array<float,3> n={bone[0]*x+bone[1]*y+bone[2]*z,bone[4]*x+bone[5]*y+bone[6]*z,bone[8]*x+bone[9]*y+bone[10]*z};
    return held ? std::array<float,3>{n[1],n[2],-n[0]} : n;
}
inline std::array<float,2> UV(unsigned flags,const std::array<float,3>& normal,float u,float v) {
    if(!(flags&2))return {u,v};
    // Chrome materials store an environment swatch, not ordinary triangle UVs; use the animated surface normal.
    float length=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
    if(length<.00001f)return {.5f,.5f};
    return {std::clamp(.5f+normal[0]/length*.5f,0.f,1.f),std::clamp(.5f-normal[1]/length*.5f,0.f,1.f)};
}
}
