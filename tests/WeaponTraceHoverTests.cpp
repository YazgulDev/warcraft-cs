#include "../src/platform/ItemHover.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>

namespace {
struct Unit {float entry;bool head=false,hit=true,hidden=false,loaded=false,visible=true,ally=false,effect=false,alive=true;};
std::map<wc3::Handle,Unit> scene;
std::vector<wc3::Handle> order,objects,items={100};std::deque<wc3::Handle> group;
bool itemOwned=false,itemVisible=true,wall=false;float gate=40,itemX=100;int nameCalls=0;bool volumeCalled=false;
bool diagnosticSeen=false;
wc3::Bits RealBits(float value){wc3::Bits bits;std::memcpy(&bits,&value,4);return bits;}
}
namespace wc3 {
float Real(Bits bits){float f;std::memcpy(&f,&bits,4);return f;}
Handle (__cdecl* CreateGroup)()=[]()->Handle{group.assign(order.begin(),order.end());return 1;};
Handle (__cdecl* GetLocalPlayer)()=[]()->Handle{return 1;};
Handle (__cdecl* GetOwningPlayer)(Handle)=[](Handle)->Handle{return 1;};
void (__cdecl* GroupEnumUnitsInRange)(Handle,float*,float*,float*,Handle)=[](Handle,float*,float*,float*,Handle){};
Handle (__cdecl* FirstOfGroup)(Handle)=[](Handle)->Handle{return group.empty() ? 0 : group.front();};
void (__cdecl* GroupRemoveUnit)(Handle,Handle)=[](Handle,Handle){group.pop_front();};
void (__cdecl* DestroyGroup)(Handle)=[](Handle){assert(group.empty());};
Bits (__cdecl* GetUnitState)(Handle,int)=[](Handle h,int)->Bits{return RealBits(scene[h].alive ? 100.f : 0.f);};
BOOL (__cdecl* IsUnitHidden)(Handle)=[](Handle h)->BOOL{return scene[h].hidden;};
BOOL (__cdecl* IsUnitLoaded)(Handle)=[](Handle h)->BOOL{return scene[h].loaded;};
BOOL (__cdecl* IsUnitVisible)(Handle,Handle)=[](Handle h,Handle)->BOOL{return scene[h].visible;};
BOOL (__cdecl* IsUnitAlly)(Handle,Handle)=[](Handle h,Handle)->BOOL{return scene[h].ally;};
bool IsNeutralUnit(Handle){return false;}
std::vector<Handle> NearbyDestructables(float,float,float){return objects;}
Bits (__cdecl* GetDestructableLife)(Handle)=[](Handle)->Bits{return RealBits(100);};
float AttackSurface(float x,float,bool){return wall && x>=32 && x<=48 ? 100.f : -100.f;}
std::vector<Handle> NearbyItems(float,float,float){return items;}
int (__cdecl* GetUnitTypeId)(Handle)=[](Handle)->int{return 1;};
int (__cdecl* GetItemTypeId)(Handle)=[](Handle h)->int{return h==100 ? 42 : 0;};
BOOL (__cdecl* IsItemOwned)(Handle)=[](Handle)->BOOL{return itemOwned;};
BOOL (__cdecl* IsItemVisible)(Handle)=[](Handle)->BOOL{return itemVisible;};
Bits (__cdecl* GetItemX)(Handle)=[](Handle)->Bits{return RealBits(itemX);};
Bits (__cdecl* GetItemY)(Handle)=[](Handle)->Bits{return RealBits(0);};
float Ground(float,float){return 0;}
std::string ItemName(Handle){++nameCalls;return u8"|cffff0000\u0420\u0443\u043d\u0430|r";}
}
namespace WeaponTargetFilter {bool PassThrough(wc3::Handle h,uintptr_t){return scene[h].effect;}}
bool UnitHitboxes::IntersectShot(wc3::Handle h,const float*,const float*,float limit,float& entry,bool& head,float,bool diagnostic) {
    diagnosticSeen=diagnostic; // The production query forwards fired-round diagnostics to the hitbox adapter.
    const auto& u=scene[h];entry=u.entry;head=u.head;return u.hit && entry<=limit;
}
bool UnitHitboxes::Intersect(wc3::Handle h,const float*,const float*,float limit,float& entry,float){volumeCalled=true;entry=scene[h].entry;return entry<=limit;}
bool DestructableHitboxes::Intersect(wc3::Handle,const float*,const float*,float limit,float& entry){entry=gate;return gate<=limit;}
int main() {
    _set_error_mode(_OUT_TO_STDERR);
    GameplaySettings settings;UnitHitboxes units;DestructableHitboxes destructables;
    float origin[]={0,0,24},ray[]={1,0,0};WeaponTrace::Options options;
    auto cast=[&]{return WeaponTrace::Cast(1,origin,ray,200,settings,units,destructables,0,options);};
    scene[2]={20,false,false};scene[3]={100,true};order={2,3};
    auto hit=cast();assert(hit.target==3 && hit.head); // Near broad-box ghost cannot steal an actual head surface.
    assert(!diagnosticSeen);options.diagnostic=true;assert(cast().target==3 && diagnosticSeen);options.diagnostic=false;
    scene[2].hit=true;hit=cast();assert(hit.target==2 && !hit.head);
    for(bool Unit::*flag:{&Unit::hidden,&Unit::loaded,&Unit::effect}){scene[2].*flag=true;assert(cast().target==3);scene[2].*flag=false;}
    scene[2].visible=false;assert(cast().target==3);scene[2].visible=true;
    scene[2].alive=false;assert(cast().target==3);scene[2].alive=true;
    scene[2].ally=true;assert(cast().target==3);options.includeAllies=true;assert(cast().target==2);options.includeAllies=false;scene[2].ally=false;
    scene[2].entry=100;order={3,2};assert(cast().target==2);order={2,3};assert(cast().target==2);
    objects={90};hit=cast();assert(hit.target==90 && hit.destructable && !hit.head);objects.clear();
    wall=true;hit=cast();assert(!hit.target && hit.distance==32 && !hit.head);wall=false;
    options.volumes=true;cast();assert(volumeCalled);options.volumes=false;
    settings.hitDetection.preciseModels=false;volumeCalled=false;cast();assert(volumeCalled);settings.hitDetection.preciseModels=true;
    // The hover path intentionally has no inventory native: even a creature without slots can inspect a rune.
    order.clear();ItemHover hover;hover.Update(1,origin,ray,settings,units,destructables,0);
    assert(hover.Item()==100 && hover.Name()==u8"\u0420\u0443\u043d\u0430" && nameCalls==1);
    hover.Update(1,origin,ray,settings,units,destructables,0);assert(nameCalls==1);
    float away[]={0,1,0};hover.Update(1,origin,away,settings,units,destructables,0);assert(hover.Name().empty());
    itemOwned=true;hover.Update(1,origin,ray,settings,units,destructables,0);assert(!hover.Item());itemOwned=false;
    itemVisible=false;hover.Update(1,origin,ray,settings,units,destructables,0);assert(!hover.Item());itemVisible=true;
    wall=true;hover.Update(1,origin,ray,settings,units,destructables,0);assert(!hover.Item());wall=false;
    scene[2].entry=20;scene[2].ally=true;order={2};hover.Update(1,origin,ray,settings,units,destructables,0);assert(!hover.Item());order.clear();
    objects={90};hover.Update(1,origin,ray,settings,units,destructables,0);assert(!hover.Item());objects.clear();
    itemX=2000;hover.Update(1,origin,ray,settings,units,destructables,0);assert(!hover.Item());itemX=100;
    settings.itemHover.enabled=false;hover.Update(1,origin,ray,settings,units,destructables,0);assert(!hover.Item());settings.itemHover.enabled=true;
    hover.Reset();hover.Update(1,origin,ray,settings,units,destructables,0);assert(hover.Item()==100 && nameCalls==2);
    std::puts("Production trace/hover: nearest surfaces, stable order, hidden cargo/effects, allied policy, terrain/gates, localized rune names and lifecycle passed.");
}
