#include "ItemPickup.hpp"

wc3::Handle ItemPickup::Nearest(wc3::Handle unit,float radius) const {
    // Native inventory capacity supports heroes and creeps with the Backpack ability alike.
    if (wc3::UnitInventorySize(unit)<=0) return 0;
    float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit));
    float best=radius*radius;wc3::Handle nearest=0;
    for (wc3::Handle item:wc3::NearbyItems(x,y,radius)) {
        if (wc3::IsItemOwned(item) || !wc3::IsItemVisible(item)) continue;
        float dx=wc3::Real(wc3::GetItemX(item))-x,dy=wc3::Real(wc3::GetItemY(item))-y;
        float distance=dx*dx+dy*dy;
        if (distance<=best) { best=distance;nearest=item; }
    }
    return nearest;
}
ItemPickupResult ItemPickup::Take(wc3::Handle unit,float radius) const {
    wc3::Handle item=Nearest(unit,radius);if (!item) return {};
    int type=wc3::GetItemTypeId(item);bool powerup=wc3::GetItemType(item)==2;
    // Native pickup preserves inventory restrictions, bonuses and map pickup events; full bags grant nothing.
    BOOL picked=wc3::UnitAddItem(unit,item);
    // Scripted powerup activation leaves a ground object; ordinary equipment must remain in the inventory.
    if (picked && powerup && wc3::GetItemTypeId(item)) wc3::RemoveItem(item);
    wc3::Log("item pickup item=%08X type=%08X accepted=%d powerup=%d",item,type,picked,powerup);
    return {true,picked!=FALSE,powerup};
}
