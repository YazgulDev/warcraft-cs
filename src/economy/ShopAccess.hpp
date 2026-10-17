#pragma once
#include "../platform/WarcraftApi.hpp"
#include <cmath>

// Native shop abilities cover map-defined stock as well as racial item shops.
namespace ShopAccess {
inline bool Merchant(wc3::Handle shop) {
    for(int ability:{0x41736964,0x41737564,0x416E6575,0x416E6532,0x41706974})
        if(wc3::GetUnitAbilityLevel(shop,ability)>0)return true;
    return false;
}
inline bool Nearby(wc3::Handle actor,wc3::Handle shop,float radius) {
    if(!actor || !shop || !wc3::GetUnitTypeId(shop) || wc3::Real(wc3::GetUnitState(shop,0))<=.405f ||
        wc3::IsUnitHidden(shop) || !wc3::IsUnitVisible(shop,wc3::GetOwningPlayer(actor)) ||
        wc3::IsUnitEnemy(shop,wc3::GetOwningPlayer(actor)) || !Merchant(shop))return false;
    return std::hypot(wc3::Real(wc3::GetUnitX(actor))-wc3::Real(wc3::GetUnitX(shop)),
        wc3::Real(wc3::GetUnitY(actor))-wc3::Real(wc3::GetUnitY(shop)))<=radius;
}
}
