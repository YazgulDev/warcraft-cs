#include "ItemHover.hpp"
#include "../geometry/RayEllipsoid.hpp"
#include "../presentation/GameText.hpp"

void ItemHover::Update(wc3::Handle actor,const float* origin,const float* direction,const GameplaySettings& settings,
    UnitHitboxes& units,DestructableHitboxes& objects,uintptr_t base) {
    Clear();if(!settings.itemHover.enabled || !actor || !wc3::GetUnitTypeId(actor))return;
    const auto& tuning=settings.itemHover;DWORD now=GetTickCount();
    // Refresh handles periodically, but test the current look every frame so old labels cannot follow a fast turn.
    if(actor_!=actor || !scanned_ || now-scanned_>=150 || std::hypot(origin[0]-scanX_,origin[1]-scanY_)>128) {
        items_=wc3::NearbyItems(origin[0],origin[1],tuning.distance+tuning.radius);
        scanned_=now ? now : 1;actor_=actor;scanX_=origin[0];scanY_=origin[1];
    }
    float nearest=tuning.distance;
    for(auto item:items_) {
        int type=wc3::GetItemTypeId(item);
        if(!type || wc3::IsItemOwned(item) || !wc3::IsItemVisible(item))continue;
        float x=wc3::Real(wc3::GetItemX(item)),y=wc3::Real(wc3::GetItemY(item)),r=tuning.radius;
        float z=wc3::Ground(x,y)+r*.75f,entry=0;
        Bounds3 bounds={{x-r,y-r,z-r},{x+r,y+r,z+r}};
        if(IntersectEllipsoid(bounds,origin,direction,nearest,entry)){nearest=entry;item_=item;}
    }
    if(!item_)return;
    // Units, gates and terrain hide names behind them. This query cannot deal damage or trigger pickup events.
    WeaponTrace::Options trace;trace.includeAllies=true;trace.surfaceOnMiss=true;
    auto blocker=WeaponTrace::Cast(actor,origin,direction,nearest,settings,units,objects,base,trace);
    if(blocker.target || blocker.distance<nearest-.001f){Clear();return;}
    int type=wc3::GetItemTypeId(item_);auto name=names_.find(type);
    if(name==names_.end())name=names_.emplace(type,GameText::Label(wc3::ItemName(item_))).first;
    name_=name->second;if(name_.empty())Clear();
}
