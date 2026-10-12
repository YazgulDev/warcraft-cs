#include "ThirdPersonCamera.hpp"
#include <algorithm>

ThirdPersonPose ThirdPersonCamera::Position(wc3::Handle actor,float z,float yaw,float pitch,const ThirdPersonSettings& settings,
    UnitHitboxes& units,DestructableHitboxes& destructables) const {
    float x=wc3::Real(wc3::GetUnitX(actor)),y=wc3::Real(wc3::GetUnitY(actor));
    auto pose=ThirdPersonPose::Calculate(x,y,z,yaw,pitch,settings.height,settings.distance);
    float direction[]={-pose.direction[0],-pose.direction[1],-pose.direction[2]},limit=settings.distance;
    // Sample the boom against terrain, then test solid model surfaces rather than canopy-sized pathing.
    for(float t=8;t<=limit;t+=8) {
        float px=x+direction[0]*t,py=y+direction[1]*t,pz=pose.pivot[2]+direction[2]*t;
        if(wc3::TerrainGround(px,py)+settings.clearance>=pz) {limit=std::max(0.f,t-8);break;}
    }
    for(auto object:wc3::NearbyDestructables(x,y,settings.distance+512)) {
        float entry=0;
        if(wc3::Real(wc3::GetDestructableLife(object))>.405f &&
            destructables.Intersect(object,pose.pivot.data(),direction,limit,entry))limit=entry;
    }
    auto group=wc3::CreateGroup();float radius=settings.distance+512;
    wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    while(auto object=wc3::FirstOfGroup(group)) {
        wc3::GroupRemoveUnit(group,object);float entry=0;
        // Ordinary moving followers must not make the camera bounce; solid buildings do shorten it.
        if(object!=actor && wc3::IsUnitType(object,2) && !wc3::IsUnitHidden(object) &&
            wc3::Real(wc3::GetUnitState(object,0))>.405f &&
            units.Intersect(object,pose.pivot.data(),direction,limit,entry,settings.clearance))limit=entry;
    }
    wc3::DestroyGroup(group);
    // Keep the requested free-space distance; clearance is reserved only when geometry shortens the boom.
    pose.Place(limit<settings.distance ? std::max(0.f,limit-settings.clearance) : limit);return pose;
}
