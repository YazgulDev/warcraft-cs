#include "NativeBlast.hpp"
#include "../combat/CombatDamage.hpp"
#include <algorithm>

namespace {
float Distance(const GrenadePoint& a,const GrenadePoint& b){float n=0;for(int i=0;i<3;++i)n+=(a[i]-b[i])*(a[i]-b[i]);return std::sqrt(n);}
bool Contains(const Bounds3& box,const GrenadePoint& p){for(int i=0;i<3;++i)if(p[i]<box.minimum[i] || p[i]>box.maximum[i])return false;return true;}
}
bool NativeBlast::Covered(const GrenadePoint& from,const GrenadePoint& to,const std::vector<Bounds3>& obstacles) {
    GrenadePoint direction{};for(int i=0;i<3;++i)direction[i]=to[i]-from[i];
    for(const auto& box:obstacles) {
        // The victim's own enclosing volume and the impact volume cannot become artificial shields.
        if(Contains(box,from) || Contains(box,to))continue;
        float entry=0;if(IntersectBounds(box,from.data(),direction.data(),1,entry))return true;
    }
    int steps=std::max(1,int(Distance(from,to)/24));
    for(int i=1;i<steps;++i){float t=float(i)/steps;if(wc3::AttackSurface(from[0]+direction[0]*t,from[1]+direction[1]*t,false)>from[2]+direction[2]*t)return true;}
    return false;
}
bool NativeBlast::Apply(wc3::Handle actor,int type,const GrenadePoint& from,float peak,float radius,float friendly,
    const std::vector<Bounds3>& obstacles,const std::function<bool()>& valid,const char* label) {
    if(!valid() || wc3::GetUnitTypeId(actor)!=type)return false;
    auto owner=wc3::GetOwningPlayer(actor),group=wc3::CreateGroup();if(!group)return false;
    float x=from[0],y=from[1];wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);std::vector<wc3::Handle> targets;
    for(auto target=wc3::FirstOfGroup(group);target;target=wc3::FirstOfGroup(group)){wc3::GroupRemoveUnit(group,target);targets.push_back(target);}
    wc3::DestroyGroup(group);bool hit=false;
    // Snapshot before native damage callbacks mutate groups; damage the owner last and stop after map reset.
    std::stable_sort(targets.begin(),targets.end(),[&](auto a,auto b){return a!=actor && b==actor;});
    for(auto target:targets) {
        if(!valid() || wc3::GetUnitTypeId(actor)!=type)return hit;
        if(!wc3::GetUnitTypeId(target) || wc3::IsUnitHidden(target) || wc3::IsUnitLoaded(target))continue;
        float before=wc3::Real(wc3::GetUnitState(target,0));if(before<=.405f)continue;
        GrenadePoint point={wc3::Real(wc3::GetUnitX(target)),wc3::Real(wc3::GetUnitY(target)),0};point[2]=wc3::Ground(point[0],point[1])+wc3::Real(wc3::GetUnitFlyHeight(target))+32;
        float falloff=GrenadeProjectile::Falloff(Distance(from,point),radius);if(falloff<=0 || Covered(from,point,obstacles))continue;
        float damage=CombatDamage::Amount(peak*falloff,wc3::IsUnitAlly(target,owner)!=FALSE,false,before,friendly);
        if(damage>0)wc3::UnitDamageTarget(actor,target,&damage,FALSE,TRUE,0,4,0);
        if(!valid())return hit;
        float after=wc3::GetUnitTypeId(target) ? wc3::Real(wc3::GetUnitState(target,0)) : 0;hit|=after<before;
        wc3::Log("%s target=%08X damage=%.1f hpBefore=%.1f hpAfter=%.1f",label,target,damage,before,after);
    }
    for(auto target:wc3::NearbyDestructables(from[0],from[1],radius)) {
        if(!valid() || wc3::GetUnitTypeId(actor)!=type)return hit;
        if(!wc3::GetDestructableTypeId(target) || wc3::IsDestructableInvulnerable(target))continue;
        float before=wc3::Real(wc3::GetDestructableLife(target));if(before<=.405f)continue;
        GrenadePoint point={wc3::Real(wc3::GetDestructableX(target)),wc3::Real(wc3::GetDestructableY(target)),0};point[2]=wc3::Ground(point[0],point[1])+32;
        float damage=peak*GrenadeProjectile::Falloff(Distance(from,point),radius);if(damage<=0 || Covered(from,point,obstacles))continue;
        wc3::UnitDamageTarget(actor,target,&damage,FALSE,TRUE,0,4,0);if(!valid())return hit;
        float after=wc3::GetDestructableTypeId(target) ? wc3::Real(wc3::GetDestructableLife(target)) : 0;hit|=after<before;
        wc3::Log("%s gate=%08X damage=%.1f hpBefore=%.1f hpAfter=%.1f",label,target,damage,before,after);
    }
    return hit;
}
