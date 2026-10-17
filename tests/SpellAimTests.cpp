#include "../src/platform/SpellAim.hpp"
#include <cassert>
#include <vector>
#include <cstdio>
#include <cmath>

namespace {std::vector<wc3::Handle> group;float wall=1000;int created=0,destroyed=0;bool unitsEnabled=true,crowded=false;}
namespace wc3 {
float Ground(float,float) {return 0;}
float Real(Bits bits) {return float(bits);}
std::vector<Handle> NearbyDestructables(float,float,float) {return {9};}
Handle (__cdecl* GetLocalPlayer)()=[]()->Handle {return 1;};
Handle (__cdecl* CreateGroup)()=[]()->Handle {++created;return 1;};
void (__cdecl* DestroyGroup)(Handle)=[](Handle) {++destroyed;};
void (__cdecl* GroupEnumUnitsInRange)(Handle,float*,float*,float*,Handle)=[](Handle,float*,float*,float*,Handle) {
    group=unitsEnabled ? std::vector<wc3::Handle>{1,2,3,4,5,6} : std::vector<wc3::Handle>{};if(crowded)group.push_back(7);
};
Handle (__cdecl* FirstOfGroup)(Handle)=[](Handle)->Handle {return group.empty() ? 0 : group.back();};
void (__cdecl* GroupRemoveUnit)(Handle,Handle)=[](Handle,Handle) {group.pop_back();};
Bits (__cdecl* GetUnitState)(Handle,int)=[](Handle h,int)->Bits {return h==6 ? 0 : 100;};
Bits (__cdecl* GetDestructableLife)(Handle)=[](Handle)->Bits {return 100;};
BOOL (__cdecl* IsUnitHidden)(Handle)=[](Handle h)->BOOL {return h==3;};
BOOL (__cdecl* IsUnitLoaded)(Handle)=[](Handle h)->BOOL {return h==4;};
BOOL (__cdecl* IsUnitVisible)(Handle,Handle)=[](Handle h,Handle)->BOOL {return h!=5;};
}
bool UnitHitboxes::Intersect(wc3::Handle unit,const float*,const float*,float limit,float& entry,float) {
    entry=unit==2 ? 80.f : 10.f;return entry<limit;
}
bool DestructableHitboxes::Intersect(wc3::Handle,const float*,const float*,float limit,float& entry) {entry=wall;return entry<limit;}
int main() {
    UnitHitboxes units;DestructableHitboxes objects;float eye[]={0,0,96},forward[]={1,0,0},down[]={.70710678f,0,-.70710678f};
    auto aim=SpellAim::Pick(1,eye,forward,100,units,objects);assert(aim.target==2 && !aim.point);
    wall=40;aim=SpellAim::Pick(1,eye,forward,100,units,objects);assert(aim.target==9 && !aim.point);wall=1000;
    aim=SpellAim::Pick(1,eye,forward,50,units,objects);assert(!aim.target && !aim.point);
    // The same terrain point survives a nearer unit/destructable; neither widget can snap an area cast.
    aim=SpellAim::Pick(1,eye,down,300,units,objects);assert(aim.target==2 && aim.point && std::abs(aim.x-96)<.1f);
    wall=40;aim=SpellAim::Pick(1,eye,down,300,units,objects);assert(aim.target==9 && aim.point && std::abs(aim.x-96)<.1f);
    wall=1000;unitsEnabled=false;aim=SpellAim::Pick(1,eye,down,300,units,objects);
    assert(!aim.target && aim.point && std::abs(aim.x-96)<.1f);assert(created==destroyed);
    // A nearby protected ally is transparent to attack picking, while ally spells and Q opt-in retain normal nearest-hit selection.
    unitsEnabled=true;crowded=true;auto enemies=[](wc3::Handle h){return h==2;};
    assert(SpellAim::Pick(1,eye,forward,100,units,objects).target==7);
    assert(SpellAim::Pick(1,eye,forward,100,units,objects,enemies).target==2);
    assert(SpellAim::Pick(1,eye,forward,100,units,objects,[](wc3::Handle){return true;}).target==7);
    wall=40;assert(SpellAim::Pick(1,eye,forward,100,units,objects,enemies).target==9);
    wall=1000;assert(!SpellAim::Pick(1,eye,forward,50,units,objects,enemies).target);
    aim=SpellAim::Pick(1,eye,down,300,units,objects,enemies);
    assert(aim.target==2 && aim.point && std::abs(aim.x-96)<.1f && created==destroyed);
    std::puts("PASS spell ray visibility, dead/hidden/loaded filtering, nearest obstruction and no sky fallback.");
}
