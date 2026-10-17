#include "SpellAim.hpp"
#include <cmath>
#include <algorithm>

SpellAim SpellAim::Pick(wc3::Handle actor,const float* origin,const float* direction,float distance,
    UnitHitboxes& units,DestructableHitboxes& destructables,const std::function<bool(wc3::Handle)>& unitFilter) {
    SpellAim aim;float limit=distance;
    // The first terrain crossing limits target picking; point casts require a real surface under the reticle.
    for(float t=0;t<=distance;t+=24) {
        float x=origin[0]+direction[0]*t,y=origin[1]+direction[1]*t,z=origin[2]+direction[2]*t;
        if(wc3::Ground(x,y)>=z) {
            float lo=std::max(0.f,t-24),hi=t;
            for(int i=0;i<8;++i) {float mid=(lo+hi)*.5f;
                if(wc3::Ground(origin[0]+direction[0]*mid,origin[1]+direction[1]*mid)>=origin[2]+direction[2]*mid)hi=mid;else lo=mid;}
            limit=hi;aim.x=origin[0]+direction[0]*hi;aim.y=origin[1]+direction[1]*hi;aim.point=true;break;
        }
    }
    float x=origin[0],y=origin[1],radius=distance+512;
    wc3::Handle group=wc3::CreateGroup(),candidate=0,local=wc3::GetLocalPlayer();
    wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    while((candidate=wc3::FirstOfGroup(group))) {
        wc3::GroupRemoveUnit(group,candidate);
        if(candidate==actor || wc3::Real(wc3::GetUnitState(candidate,0))<=.405f || wc3::IsUnitHidden(candidate) ||
            wc3::IsUnitLoaded(candidate) || !wc3::IsUnitVisible(candidate,local))continue;
        // Rejected allies must not shorten an attack ray and hide a valid enemy behind them; spells accept allies by default.
        if(unitFilter && !unitFilter(candidate))continue;
        float entry=0;if(units.Intersect(candidate,origin,direction,limit,entry)) {limit=entry;aim.target=candidate;}
    }
    wc3::DestroyGroup(group);
    for(wc3::Handle object:wc3::NearbyDestructables(x,y,radius)) {
        if(wc3::Real(wc3::GetDestructableLife(object))<=.405f)continue;
        float entry=0;if(destructables.Intersect(object,origin,direction,limit,entry)) {limit=entry;aim.target=object;}
    }
    // Keep the ground intersection independent: a nearby ally's model must not shorten Blink/area casts.
    return aim;
}
