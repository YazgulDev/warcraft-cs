#include "../src/squad/SquadController.hpp"
#include "../src/platform/FpsCombatGuard.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <vector>

namespace {
struct Unit {
    alignas(uintptr_t) std::array<unsigned char,0x200> objects[2]{};
    int generation=0,order=0,owner=10;float hp=100,x=0,speed=270;
    uintptr_t target=0;bool exists=true,building=false,paused=false,hidden=false,loaded=false,visible=true,accept=true;
};
std::array<Unit,13> units;std::vector<wc3::Handle> group;int targets=0,stops=0,points=0,spellTicks=0;
SquadController* squad=nullptr;
wc3::Bits Encoded(float value) {wc3::Bits bits;memcpy(&bits,&value,4);return bits;}
wc3::Handle __cdecl Group() {group.clear();return 1;}
void __cdecl Destroy(wc3::Handle) {}
void __cdecl EnumAll(wc3::Handle,wc3::Handle,wc3::Handle) {for(unsigned i=1;i<units.size();++i)group.push_back(i);}
void __cdecl EnumRange(wc3::Handle,float*,float*,float* radius,wc3::Handle) {for(unsigned i=1;i<units.size();++i)if(units[i].x<=*radius)group.push_back(i);}
wc3::Handle __cdecl First(wc3::Handle) {return group.empty() ? 0 : group.back();}
void __cdecl Remove(wc3::Handle,wc3::Handle) {group.pop_back();}
BOOL __cdecl Target(wc3::Handle unit,int order,wc3::Handle target) {
    ++targets;
    // This is the real callback boundary: passive guards must already permit the explicit order.
    assert(!squad->BlocksUnit(wc3::UnitObject(unit)) && !squad->BlocksAttack(100+unit));
    if(!units[unit].accept)return FALSE;
    units[unit].order=order;units[unit].target=wc3::UnitObject(target);return TRUE;
}
BOOL __cdecl Stop(wc3::Handle unit,int order) {++stops;units[unit].order=order;units[unit].target=0;return TRUE;}
BOOL __cdecl Point(wc3::Handle unit,int order,float*,float*) {++points;units[unit].order=order;return TRUE;}
}
namespace wc3 {
void Log(const char*,...) {}
float Real(Bits bits) {float f;memcpy(&f,&bits,4);return f;}
uintptr_t UnitObject(Handle unit) {return unit && unit<units.size() && units[unit].exists ? uintptr_t(units[unit].objects[units[unit].generation].data()) : 0;}
bool IsNeutralUnit(Handle) {return false;}
Handle (__cdecl* GetLocalPlayer)()=[]()->Handle{return 10;};
Handle (__cdecl* GetOwningPlayer)(Handle)=[](Handle u)->Handle{return units[u].owner;};
int (__cdecl* GetUnitTypeId)(Handle)=[](Handle u){return UnitObject(u) ? 123 : 0;};
Bits (__cdecl* GetUnitState)(Handle,int)=[](Handle u,int){return Encoded(units[u].hp);};
Bits (__cdecl* GetUnitX)(Handle)=[](Handle u){return Encoded(units[u].x);};
Bits (__cdecl* GetUnitY)(Handle)=[](Handle){return Encoded(0);};
Bits (__cdecl* GetUnitMoveSpeed)(Handle)=[](Handle u){return Encoded(units[u].speed);};
BOOL (__cdecl* IsUnitType)(Handle,int)=[](Handle u,int)->BOOL{return units[u].building;};
BOOL (__cdecl* IsUnitPaused)(Handle)=[](Handle u)->BOOL{return units[u].paused;};
BOOL (__cdecl* IsUnitHidden)(Handle)=[](Handle u)->BOOL{return units[u].hidden;};
BOOL (__cdecl* IsUnitLoaded)(Handle)=[](Handle u)->BOOL{return units[u].loaded;};
BOOL (__cdecl* IsUnitVisible)(Handle,Handle)=[](Handle u,Handle)->BOOL{return units[u].visible;};
BOOL (__cdecl* IsUnitEnemy)(Handle,Handle)=[](Handle u,Handle p)->BOOL{return unsigned(units[u].owner)!=p && units[u].owner!=12;};
Handle (__cdecl* CreateGroup)()=Group;
void (__cdecl* DestroyGroup)(Handle)=Destroy;
void (__cdecl* GroupEnumUnitsOfPlayer)(Handle,Handle,Handle)=EnumAll;
void (__cdecl* GroupEnumUnitsInRange)(Handle,float*,float*,float*,Handle)=EnumRange;
Handle (__cdecl* FirstOfGroup)(Handle)=First;
void (__cdecl* GroupRemoveUnit)(Handle,Handle)=Remove;
int (__cdecl* GetUnitCurrentOrder)(Handle)=[](Handle u){return units[u].order;};
BOOL (__cdecl* IssueTargetOrderById)(Handle,int,Handle)=Target;
BOOL (__cdecl* IssueImmediateOrderById)(Handle,int)=Stop;
BOOL (__cdecl* IssuePointOrderById)(Handle,int,float*,float*)=Point;
}
namespace FpsCombatGuard {
uintptr_t AttackTargetObject(wc3::Handle u) {return units[u].target;}
bool HasAttackTarget(wc3::Handle u) {return units[u].target!=0;}
void CancelAttacks(wc3::Handle u) {if(units[u].target)Stop(u,851972);}
}
bool SquadSpellcasting::Tick(wc3::Handle,const GameplaySettings&,DWORD) {++spellTicks;return false;}
void SquadSpellcasting::Cancel(wc3::Handle) {}
int main() {
    for(unsigned i=1;i<units.size();++i)*reinterpret_cast<uintptr_t*>(units[i].objects[0].data()+0x1E8)=100+i;
    units[2].x=-300;units[3].x=2400;units[4].owner=11;units[5].owner=11;
    units[6].building=true;units[7].loaded=true;units[8].hp=0;units[9].owner=12;
    units[10].paused=true;units[11].speed=0;units[12].visible=false;
    GameplaySettings settings;settings.squadMaxUnits=1;SquadController controller;squad=&controller;
    controller.Capture(1,true,settings);assert(controller.Count()==1 && controller.Passive());
    assert(controller.BlocksUnit(wc3::UnitObject(2)) && controller.BlocksAttack(102));
    // RMB reaches the distant owned unit without changing H/O membership, mode or native combat rules.
    assert(controller.Focus(1,4,settings,1000)==2 && targets==2 && controller.Count()==1 && controller.Passive());
    int stopped=stops;controller.Tick(1,settings,1200,true,0,0,0);
    assert(stops==stopped && points==0 && spellTicks==0 && units[3].order==851983);
    // Empty aim cancels both owned attacks and restores the exact passive policy.
    assert(controller.Focus(1,0,settings,1300)==0 && stops==stopped+2);
    assert(controller.BlocksUnit(wc3::UnitObject(2)) && controller.BlocksAttack(102));
    controller.Tick(1,settings,1500,true,0,0,0);assert(points==1);
    // Switching targets, external orders and ownership/handle reuse never stop unrelated native work.
    assert(controller.Focus(1,4,settings,2000)==2);units[2].order=851986;units[2].target=0;
    units[3].target=wc3::UnitObject(5);controller.Tick(1,settings,2200,true,0,0,0);
    stopped=stops;controller.CancelFocus();assert(stops==stopped);
    assert(controller.Focus(1,4,settings,3000)==2);units[3].owner=11;units[2].generation=1;
    stopped=stops;controller.CancelFocus();assert(stops==stopped);units[3].owner=10;units[2].generation=0;
    assert(controller.Focus(1,4,settings,4000)==2);units[4].hp=0;
    stopped=stops;controller.Tick(1,settings,4200,true,0,0,0);assert(stops==stopped+2);units[4].hp=100;
    // Invalid/allied targets, native rejection, F8 opt-out and map teardown have independent cleanup paths.
    assert(controller.Focus(1,9,settings,5000)==0 && controller.Focus(1,1,settings,5000)==0);
    units[3].accept=false;assert(controller.Focus(1,4,settings,5100)==1);units[3].accept=true;
    settings.squadReticleAttack=false;stopped=stops;controller.Tick(1,settings,5300,true,0,0,0);assert(stops==stopped+1);
    settings.squadReticleAttack=true;assert(controller.Focus(1,4,settings,6000)==2);
    stopped=stops;controller.Reset();assert(stops==stopped && controller.Count()==0);
}
