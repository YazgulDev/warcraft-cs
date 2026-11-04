#pragma once
#include "../geometry/CylinderMesh.hpp"
#include "../movement/MovementPhysics.hpp"

// Only decoded stationary solid faces participate; render bounds never become a fake surf plane.
namespace ModelSurfContact {
using Meshes=std::vector<const CylinderMesh*>;
inline MeshSurfaceRange Range(const MovementPhysics& movement) {
    return movement.SurfModelContacts() ? MeshSurfaceRange{
        std::cos(movement.SurfMaximumAngle()*.01745329252f),std::cos(movement.MaxWalkSlope()*.01745329252f)} : MeshSurfaceRange{};
}
inline MovementSurface Sample(float x,float y,float radius,const MovementPhysics& movement,const Meshes& meshes) {
    MovementSurface result;result.walkable=false;
    if(!movement.SurfModelContacts() || movement.Swimming())return result;
    auto range=Range(movement);
    float reachable=std::max(movement.FeetZ(),movement.PreviousFeetZ())+movement.StepHeight();
    for(const auto* mesh:meshes)if(mesh) {
        float height=0,normal[3]={};
        // Use the actual highest foot contact, not a triangle artificially cut at reachable Z.
        if(mesh->Floor(x,y,radius,reachable,range.minimumNormal,height,normal,false) &&
            normal[2]<range.maximumNormal && (!result.walkable || height>result.height)) {
            result.height=height;result.walkable=result.ramp=true;
            std::copy(normal,normal+3,result.normal);
        }
    }
    return result;
}
inline bool Resolve(float x,float y,float radius,MovementPhysics& movement,const Meshes& meshes) {
    auto surface=Sample(x,y,radius,movement,meshes);
    return surface.walkable && movement.ResolveSurf(surface); // The caller still owns full body/head clearance.
}
inline bool OwnsFloor(float x,float y,float radius,float floor,MovementPhysics& movement,const Meshes& meshes) {
    auto surface=Sample(x,y,radius,movement,meshes);
    if(!surface.walkable || floor>surface.height+.01f ||
        surface.height<std::min(movement.FeetZ(),movement.PreviousFeetZ())-movement.SurfContactDistance()-.01f)return false;
    movement.BeginFall();return true;
}
inline bool PathCell(const Bounds3& box,float x,float y) {
    float left=std::floor(x/32)*32,bottom=std::floor(y/32)*32;
    return left<box.maximum[0] && left+32>box.minimum[0] && bottom<box.maximum[1] && bottom+32>box.minimum[1];
}
}
