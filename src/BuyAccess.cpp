#include "BuyAccess.hpp"
#include <cmath>
#include <sstream>
#include <algorithm>

namespace BuyAccess {
bool Allowed(wc3::Handle actor,const GameplaySettings& settings) {
    if (settings.buyAccess==GameplaySettings::BuyAccess::Anywhere) return true;
    if (!actor || !wc3::GetUnitTypeId(actor)) return false;
    float x=wc3::Real(wc3::GetUnitX(actor)),y=wc3::Real(wc3::GetUnitY(actor)),radius=settings.buyRadius;
    auto owner=wc3::GetOwningPlayer(actor);
    auto group=wc3::CreateGroup();if (!group) return false;
    wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    bool allowed=false;wc3::Handle target=0;
    while ((target=wc3::FirstOfGroup(group))) {
        wc3::GroupRemoveUnit(group,target);
        if (wc3::Real(wc3::GetUnitState(target,0))<=.405f || wc3::IsUnitHidden(target) || !wc3::IsUnitVisible(target,owner)) continue;
        bool friendly=wc3::IsUnitAlly(target,owner)!=FALSE;
        if (!friendly && wc3::GetOwningPlayer(target)!=wc3::Player(15)) continue;
        // Stock neutral merchants use Aneu/Apit and mercenary camps Ane2; racial/custom shops may use Asid/Asud.
        const int abilities[]={0x41736964,0x41737564,0x416E6575,0x416E6532,0x41706974};
        bool shop=false;
        for (int ability:abilities) if (wc3::GetUnitAbilityLevel(target,ability)>0) shop=true;
        std::string raw=settings.shopTypes;std::replace(raw.begin(),raw.end(),',',' ');
        std::istringstream codes(raw);std::string code;
        while (!shop && codes>>code) if (code.size()==4) {
            unsigned type=0;for (unsigned char c:code) type=(type<<8)|c;
            shop=unsigned(wc3::GetUnitTypeId(target))==type;
        }
        float dx=wc3::Real(wc3::GetUnitX(target))-x,dy=wc3::Real(wc3::GetUnitY(target))-y;
        // Logical center distance is explicit/configurable; a neutral building alone is never a buy zone.
        if (dx*dx+dy*dy<=radius*radius && (shop ||
            (settings.buyAccess==GameplaySettings::BuyAccess::FriendlyBuildings && friendly && wc3::IsUnitType(target,2)))) allowed=true;
    }
    wc3::DestroyGroup(group);return allowed;
}
}
