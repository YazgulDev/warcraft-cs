#include "../src/economy/BuyAccess.hpp"
#include <cassert>
#include <cstring>
#include <vector>

namespace {
struct Unit { wc3::Handle owner;float x,y,hp;bool building,hidden,visible;int type,ability; };
Unit units[]={{1,0,0,100,false,false,true,0x68666F6F,0},{1,100,0,100,true,false,true,0x68686F75,0}};
std::vector<wc3::Handle> group;
wc3::Bits Bits(float value) { wc3::Bits result;memcpy(&result,&value,4);return result; }
}
namespace wc3 {
float Real(Bits value) { float result;memcpy(&result,&value,4);return result; }
Handle (__cdecl* GetOwningPlayer)(Handle)=[](Handle unit){return units[unit-1].owner;};
int (__cdecl* GetUnitTypeId)(Handle)=[](Handle unit){return units[unit-1].type;};
Bits (__cdecl* GetUnitX)(Handle)=[](Handle unit){return ::Bits(units[unit-1].x);};
Bits (__cdecl* GetUnitY)(Handle)=[](Handle unit){return ::Bits(units[unit-1].y);};
Bits (__cdecl* GetUnitState)(Handle,int)=[](Handle unit,int){return ::Bits(units[unit-1].hp);};
BOOL (__cdecl* IsUnitHidden)(Handle)=[](Handle unit)->BOOL{return units[unit-1].hidden;};
BOOL (__cdecl* IsUnitVisible)(Handle,Handle)=[](Handle unit,Handle)->BOOL{return units[unit-1].visible;};
BOOL (__cdecl* IsUnitAlly)(Handle,Handle)=[](Handle unit,Handle owner)->BOOL{return units[unit-1].owner==owner;};
BOOL (__cdecl* IsUnitType)(Handle,int)=[](Handle unit,int)->BOOL{return units[unit-1].building;};
int (__cdecl* GetUnitAbilityLevel)(Handle,int)=[](Handle unit,int ability){return units[unit-1].ability==ability ? 1 : 0;};
Handle (__cdecl* Player)(int)=[](int player)->Handle{return unsigned(player);};
Handle (__cdecl* CreateGroup)()=[]()->Handle{group={2};return 1;};
void (__cdecl* GroupEnumUnitsInRange)(Handle,float*,float*,float*,Handle)=[](Handle,float*,float*,float*,Handle){};
Handle (__cdecl* FirstOfGroup)(Handle)=[](Handle)->Handle{return group.empty() ? 0 : group.front();};
void (__cdecl* GroupRemoveUnit)(Handle,Handle)=[](Handle,Handle){group.erase(group.begin());};
void (__cdecl* DestroyGroup)(Handle)=[](Handle){group.clear();};
}
int main() {
    GameplaySettings settings;
    assert(BuyAccess::Allowed(1,settings));
    settings.buyAccess=GameplaySettings::BuyAccess::Shops;assert(!BuyAccess::Allowed(1,settings));
    settings.buyAccess=GameplaySettings::BuyAccess::FriendlyBuildings;assert(BuyAccess::Allowed(1,settings));
    units[1].owner=2;assert(!BuyAccess::Allowed(1,settings));
    units[1].owner=15;
    // Stock neutral camps/merchants, racial shops and custom rawcodes are valid; hostile shops are not.
    settings.buyAccess=GameplaySettings::BuyAccess::Shops;
    for (int ability:{0x41736964,0x41737564,0x416E6575,0x416E6532,0x41706974}) {
        units[1].ability=ability;assert(BuyAccess::Allowed(1,settings));
    }
    units[1].owner=2;assert(!BuyAccess::Allowed(1,settings));
    units[1].owner=15;units[1].hidden=true;assert(!BuyAccess::Allowed(1,settings));
    units[1].hidden=false;units[1].visible=false;assert(!BuyAccess::Allowed(1,settings));
    units[1].visible=true;units[1].hp=0;assert(!BuyAccess::Allowed(1,settings));
    units[1].hp=100;units[1].x=601;assert(!BuyAccess::Allowed(1,settings));
    units[1].x=600;assert(BuyAccess::Allowed(1,settings));
    units[1].ability=0;settings.shopTypes="ngme,hhou";assert(BuyAccess::Allowed(1,settings));
    settings.shopTypes="ngme";assert(!BuyAccess::Allowed(1,settings));
}
