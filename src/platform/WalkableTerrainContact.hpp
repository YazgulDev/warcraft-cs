#pragma once
#include "WarcraftApi.hpp"
#include "../movement/MovementSurface.hpp"
#include <algorithm>
#include <cmath>

// A local, native-walkable ramp can support an airborne foot; a terrace top or blocked cliff cannot.
namespace WalkableTerrainContact {
inline MovementSurface Sample(float x,float y,float maximumSlope,float velocityX,float velocityY) {
    MovementSurface surface;surface.walkable=false;surface.height=wc3::TerrainGround(x,y);
    if(!std::isfinite(surface.height) || wc3::IsTerrainPathable(&x,&y,1))return surface;
    int level=wc3::GetTerrainCliffLevel(&x,&y);float gradient[2]={},bound[2]={};
    constexpr float probe=8;
    for(int axis=0;axis<2;++axis) {
        float dx=axis ? 0 : probe,dy=axis ? probe : 0;
        float ax=x-dx,ay=y-dy,bx=x+dx,by=y+dy;
        float before=wc3::TerrainGround(ax,ay),after=wc3::TerrainGround(bx,by);
        if(!std::isfinite(before) || !std::isfinite(after) ||
            wc3::GetTerrainCliffLevel(&ax,&ay)!=level || wc3::GetTerrainCliffLevel(&bx,&by)!=level ||
            wc3::IsTerrainPathable(&ax,&ay,1) || wc3::IsTerrainPathable(&bx,&by,1))return surface;
        float left=(surface.height-before)/probe,right=(after-surface.height)/probe;
        // Use both sides for the slope bound: averaging a sharp step must not turn it into a gentle ramp.
        // At a crest the foot may have crossed the peak inside this step. The incoming side
        // supplies the contact plane; averaging opposite slopes would erase that collision normal.
        float velocity=axis ? velocityY : velocityX;
        gradient[axis]=std::abs(velocity)<.001f ? (left+right)*.5f : velocity>0 ? left : right;
        bound[axis]=std::max(std::abs(left),std::abs(right));
    }
    float slope=std::hypot(bound[0],bound[1]);
    if(slope<.001f || slope>std::tan(maximumSlope*.01745329252f))return surface;
    float length=std::sqrt(1+gradient[0]*gradient[0]+gradient[1]*gradient[1]);
    surface.normal[0]=-gradient[0]/length;surface.normal[1]=-gradient[1]/length;surface.normal[2]=1/length;
    surface.walkable=surface.ramp=true;return surface;
}
}
