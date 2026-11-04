#pragma once
#include "WalkableTerrainContact.hpp"
#include "../movement/MovementPhysics.hpp"
#include "../geometry/RayBounds.hpp"
#include <vector>

// Terrain surf never opens native blocked cells, cliff transitions, vertical walls or model volumes.
namespace TerrainSurfContact {
inline MovementSurface Sample(float x,float y,float radius,const MovementPhysics& movement) {
    MovementSurface result;result.walkable=false;
    if(!movement.SurfEnabled() || movement.Swimming() || movement.Wet())return result;
    float highest=-1e30f,px=x,py=y;
    for(int point=0;point<9;++point) {
        float angle=(point-1)*.78539816339f,sx=x+(point ? std::cos(angle)*radius : 0),sy=y+(point ? std::sin(angle)*radius : 0);
        float height=wc3::TerrainGround(sx,sy);
        if(!std::isfinite(height))return result;
        if(height>highest){highest=height;px=sx;py=sy;}
    }
    result=WalkableTerrainContact::Sample(px,py,movement.SurfMaximumAngle(),movement.VelocityX(),movement.VelocityY());
    if(!result.walkable || result.normal[2]>=std::cos(movement.MaxWalkSlope()*.01745329252f)){result.walkable=false;return result;}
    // A sharp terrace lip has one flat side and one height discontinuity. It is not a surf
    // plane, even when an averaged derivative happens to fit the configured angle range.
    for(int axis=0;axis<2;++axis) {
        float dx=axis ? 0.f : 8.f,dy=axis ? 8.f : 0.f;
        float a=wc3::TerrainGround(px-dx,py-dy),b=wc3::TerrainGround(px+dx,py+dy);
        if(std::abs(a+b-2*result.height)>4){result.walkable=false;return result;}
    }
    return result;
}
inline bool Resolve(float x,float y,float radius,MovementPhysics& movement,const std::vector<Bounds3>& obstacles) {
    auto surface=Sample(x,y,radius,movement);if(!surface.walkable)return false;
    float top=surface.height+movement.BodyHeight();
    for(const auto& box:obstacles)if(x+radius>box.minimum[0] && x-radius<box.maximum[0] &&
        y+radius>box.minimum[1] && y-radius<box.maximum[1] && box.maximum[2]>surface.height+.01f && box.minimum[2]<top-.01f)return false;
    for(int point=0;point<9;++point){float a=(point-1)*.78539816339f,deck=0,px=x+(point ? std::cos(a)*radius : 0),py=y+(point ? std::sin(a)*radius : 0);
        if(wc3::WalkableSurface(px,py,deck) && deck>movement.FeetZ()+.01f && deck<top-.01f)return false;}
    return movement.ResolveSurf(surface);
}
inline bool OwnsFloor(float x,float y,float radius,float floor,MovementPhysics& movement) {
    auto surface=Sample(x,y,radius,movement);
    if(!surface.walkable || floor>surface.height+.01f ||
        surface.height>std::max(movement.FeetZ(),movement.PreviousFeetZ())+movement.StepHeight()+.01f ||
        surface.height<std::min(movement.FeetZ(),movement.PreviousFeetZ())-movement.SurfContactDistance()-.01f)return false;
    movement.BeginFall();return true; // Steep support is air, never an autojumpable ground frame.
}
}
