#include "WeaponTrace.hpp"
#include "WeaponTargetFilter.hpp"
#include "AttackTargetPolicy.hpp"
#include <cmath>

WeaponTrace::Contact WeaponTrace::Cast(wc3::Handle actor,const float* origin,const float* direction,float distance,
    const GameplaySettings& settings,UnitHitboxes& units,DestructableHitboxes& objects,uintptr_t base,Options options) {
    Contact result;result.distance=distance;
    float x=origin[0],y=origin[1],radius=distance+512;auto group=wc3::CreateGroup(),local=wc3::GetLocalPlayer(),owner=wc3::GetOwningPlayer(actor);
    wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);wc3::Handle candidate=0;
    while((candidate=wc3::FirstOfGroup(group))) {
        wc3::GroupRemoveUnit(group,candidate);
        // Cargo/hidden helpers are not visible physical shields, even if their owner/position is visible.
        if(candidate==actor || wc3::Real(wc3::GetUnitState(candidate,0))<=.405f || wc3::IsUnitHidden(candidate) ||
            wc3::IsUnitLoaded(candidate) || !wc3::IsUnitVisible(candidate,local))continue;
        if(!options.includeAllies && !settings.targetAllies && AttackTargetPolicy::Allied(candidate,owner))continue;
        if(settings.ignorePassableInvulnerableObjects && WeaponTargetFilter::PassThrough(candidate,base))continue;
        float entry=0,limit=std::min(distance,result.distance+.001f);bool head=false;
        bool hit=options.volumes || !settings.hitDetection.preciseModels ? units.Intersect(candidate,origin,direction,limit,entry,options.padding) :
            units.IntersectShot(candidate,origin,direction,limit,entry,head);
        // Stable ties do not depend on group enumeration order; a real nearer surface always wins.
        if(hit && (entry<result.distance-.0001f || (std::abs(entry-result.distance)<.0001f && candidate<result.target)))
            result={candidate,entry,false,head};
    }
    wc3::DestroyGroup(group);
    for(auto object:wc3::NearbyDestructables(x,y,radius)) {
        if(wc3::Real(wc3::GetDestructableLife(object))<=.405f)continue;
        float entry=0;if(objects.Intersect(object,origin,direction,result.distance,entry))result={object,entry,true,false};
    }
    // Preserve the existing weapon surface policy. Item labels also query a miss so terrain can hide an item.
    if(result.target || options.surfaceOnMiss || options.volumes)for(float t=16;t<result.distance;t+=16) {
        if(wc3::AttackSurface(x+direction[0]*t,y+direction[1]*t,settings.waterBlocksAttacks)>origin[2]+direction[2]*t) {
            result={0,t,false,false};break;
        }
    }
    return result;
}
