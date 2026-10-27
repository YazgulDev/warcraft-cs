#include "../src/squad/ArmyAI.hpp"
#include "../src/economy/ShopAccess.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <cmath>
#include <vector>
#include <tuple>

namespace {
struct Unit {
    alignas(uintptr_t) std::array<unsigned char,0x200> object{};
    int type=0,order=0,owner=1;float x=0,y=0,hp=100;
    bool building=false,hidden=false,loaded=false,paused=false,hero=false,merchant=false,entangle=false;
    uintptr_t target=0;
};
std::array<Unit,12> units;
std::vector<wc3::Handle> group,trees;
std::vector<std::tuple<wc3::Handle,int,wc3::Handle>> orders;
std::vector<std::tuple<wc3::Handle,float,float>> destinations;
bool buildAccepted=true,immediateAccepted=true;int builds=0,rejectSites=0;float builtX=0,builtY=0;
std::function<void()> onBuild;
wc3::Bits Bits(float f){wc3::Bits b;memcpy(&b,&f,4);return b;}
void Setup() {
    units={};trees.clear();orders.clear();destinations.clear();builds=0;rejectSites=0;buildAccepted=immediateAccepted=true;onBuild={};
    for(unsigned i=1;i<units.size();++i) {units[i].type=ArmyPlan::Code("hfoo");units[i].x=1800;*reinterpret_cast<uintptr_t*>(units[i].object.data()+0x1E8)=123;}
    units[1].hero=true;units[1].x=0;
    units[2].type=ArmyPlan::Code("htow");units[2].building=true;units[2].x=-800;
    units[3].type=ArmyPlan::Code("hpea");units[3].x=-800;
    units[4].type=ArmyPlan::Code("hbar");units[4].building=true;
    units[5].owner=2;units[5].x=5000;
}
bool Touched(wc3::Handle u) {for(auto o:orders)if(std::get<0>(o)==u)return true;return false;}
}
namespace wc3 {
#define NATIVE(result,name,arguments,offset) result (__cdecl* name) arguments=nullptr;
#include "../src/platform/NativeOffsets.inc"
#undef NATIVE
void Log(const char*,...) {}
float Real(Bits b){float f;memcpy(&f,&b,4);return f;}
uintptr_t UnitObject(Handle u){return u && u<units.size() && units[u].type ? uintptr_t(units[u].object.data()) : 0;}
int OrderId(const char* name){return !strcmp(name,"harvest") ? 852018 : !strcmp(name,"entangle") ? 852147 : 852020;}
std::vector<Handle> NearbyDestructables(float,float,float){return trees;}
}
namespace FpsCombatGuard {uintptr_t AttackTargetObject(wc3::Handle u){return units[u].target;}}
int main() {
    _set_error_mode(_OUT_TO_STDERR);
    using namespace wc3;
    CreateGroup=[]()->Handle{group.clear();return 1;};DestroyGroup=[](Handle){};
    GroupEnumUnitsOfPlayer=[](Handle,Handle owner,Handle){for(unsigned i=1;i<units.size();++i)if(unsigned(units[i].owner)==owner && units[i].type)group.push_back(i);};
    GroupEnumUnitsInRange=[](Handle,float* x,float* y,float* r,Handle){for(unsigned i=1;i<units.size();++i)if(std::hypot(units[i].x-*x,units[i].y-*y)<=*r)group.push_back(i);};
    FirstOfGroup=[](Handle)->Handle{return group.empty() ? 0 : group.back();};GroupRemoveUnit=[](Handle,Handle){group.pop_back();};
    GetUnitTypeId=[](Handle u){return u<units.size() ? units[u].type : 0;};
    GetOwningPlayer=[](Handle u)->Handle{return units[u].owner;};
    GetLocalPlayer=[]()->Handle{return 1;};
    GetUnitState=[](Handle u,int){return ::Bits(units[u].hp);};
    GetDestructableLife=[](Handle){return ::Bits(100.f);};
    GetUnitX=[](Handle u){return ::Bits(units[u].x);};GetUnitY=[](Handle u){return ::Bits(units[u].y);};
    GetUnitMoveSpeed=[](Handle){return ::Bits(270);};GetUnitCurrentOrder=[](Handle u){return units[u].order;};
    IsUnitHidden=[](Handle u)->BOOL{return units[u].hidden;};IsUnitPaused=[](Handle u)->BOOL{return units[u].paused;};IsUnitLoaded=[](Handle u)->BOOL{return units[u].loaded;};
    IsUnitType=[](Handle u,int type)->BOOL{return type==2 ? units[u].building : type==0 && units[u].hero;};
    IsUnitVisible=[](Handle,Handle)->BOOL{return TRUE;};IsUnitEnemy=[](Handle u,Handle p)->BOOL{return unsigned(units[u].owner)!=p;};
    GetPlayerState=[](Handle,int state){return state==5 ? 20 : state==4 ? 19 : 1000;};
    GetUnitAbilityLevel=[](Handle u,int ability){return ability==ArmyPlan::Code("Aent") ? int(units[u].entangle) : int(units[u].merchant);};
    IssueBuildOrderById=[](Handle u,int type,float* x,float* y)->BOOL {++builds;builtX=*x;builtY=*y;if(!buildAccepted || builds<=rejectSites)return FALSE;orders.emplace_back(u,type,0);units[u].order=type;if(onBuild)onBuild();return TRUE;};
    IssueImmediateOrderById=[](Handle u,int type)->BOOL {if(!immediateAccepted)return FALSE;orders.emplace_back(u,type,0);units[u].order=type;return TRUE;};
    IssueTargetOrderById=[](Handle u,int type,Handle t)->BOOL {orders.emplace_back(u,type,t);units[u].order=type;return TRUE;};
    IssuePointOrderById=[](Handle u,int type,float* x,float* y)->BOOL {destinations.emplace_back(u,*x,*y);orders.emplace_back(u,type,0);units[u].order=type;return TRUE;};
    Setup();ArmyAI ai;ArmyAISettings s;s.defenders=0;
    auto reserved=[](Handle u){return u==6;};
    ai.Tick(1,s,1000,reserved);assert(orders.empty()); // No resource spending before the deliberate U toggle.
    ai.Toggle();units[7].order=123456;units[8].target=555;units[9].hidden=true;units[10].loaded=true;units[11].paused=true;
    ai.Tick(1,s,1000,reserved);
    assert(Touched(2) && Touched(3) && Touched(4));
    assert(!Touched(1) && !Touched(5) && !Touched(6) && !Touched(7) && !Touched(8) && !Touched(9) && !Touched(10) && !Touched(11));
    orders.clear();ai.Tick(1,s,1100,reserved);assert(orders.empty());
    // Warcraft reports zero on some actively training buildings; accepted production must not be enqueued again.
    units[2].order=0;units[4].order=0;orders.clear();
    ai.Tick(1,s,2600,reserved);assert(!Touched(2) && !Touched(4));
    // A queued construction lease prevents duplicate sites until its building actually appears.
    units[3].order=0;ai.Tick(1,s,4500,reserved);assert(builds==1);
    ai.Toggle();orders.clear();ai.Tick(1,s,6500,reserved);assert(orders.empty());
    // Scripted ownership transfer of the actor cannot delegate another player's entire army.
    ai.Toggle();units[1].owner=2;ai.Tick(1,s,8500,reserved);assert(orders.empty());
    // Base threats pull idle reserves into defense; native targets and manually assigned followers keep priority.
    Setup();ai.Reset();ai.Toggle();s.workers=s.soldiers=s.supplyBuildings=0;
    units[5].x=-700;ai.Tick(1,s,1000,reserved);
    bool defended=false;for(auto o:orders)if(std::get<0>(o)>=7 && std::get<1>(o)==851983 && std::get<2>(o)==5)defended=true;
    assert(defended && !Touched(1) && !Touched(6));
    // U alone never rallies ordinary troops to the actor; a reserve may return to the actual hall.
    Setup();ai.Reset();ai.Toggle();s=ArmyAISettings();s.workers=s.soldiers=s.supplyBuildings=0;
    assert(!s.followActor);ai.Tick(1,s,1000,reserved);
    assert(destinations.size()==3);for(auto point:destinations)assert(std::get<1>(point)==-800);
    // Without a hall even reserve units remain in place, while the economy remains independently available.
    Setup();ai.Reset();ai.Toggle();units[2].type=0;
    ai.Tick(1,s,1000,reserved);assert(destinations.empty());
    // F8 opt-out stops only owned approaches: reserved, manually ordered and fighting units keep control.
    Setup();ai.Reset();ai.Toggle();s.followActor=true;s.defenders=0;ai.Tick(1,s,1000,reserved);
    s.followActor=false;units[7].order=123456;units[8].target=555;orders.clear();
    ai.Tick(1,s,3000,reserved);assert(!Touched(6) && !Touched(7) && !Touched(8));
    assert(Touched(9) && units[9].order==851972);
    orders.clear();ai.Tick(1,s,5000,reserved);assert(!Touched(9));
    // Workers hidden inside a mine still satisfy the worker target and cannot cause endless replacement training.
    Setup();ai.Reset();ai.Toggle();s.workers=6;
    for(unsigned i=7;i<units.size();++i) {units[i].type=ArmyPlan::Code("hpea");units[i].hidden=true;}
    ai.Tick(1,s,1000,reserved);assert(!Touched(2));
    // Reinforcements park within arrival distance, while a native fight keeps its target/order untouched.
    Setup();ai.Reset();ai.Toggle();s.workers=s.soldiers=s.supplyBuildings=0;s.defenders=0;s.followActor=true;
    units[7].hero=true; // An unassigned second hero is eligible; only the controlled actor and H/O membership reserve it.
    ai.Tick(1,s,1000,reserved);assert(Touched(7) && Touched(8));
    units[7].x=100;units[8].x=100;units[8].target=555;orders.clear();
    ai.Tick(1,s,3000,reserved);assert(units[7].order==851972 && !Touched(8));
    // Synchronous map order callbacks can transfer another unit or the actor before the next AI command.
    Setup();ai.Reset();ai.Toggle();s=ArmyAISettings();s.defenders=0;s.followActor=true;
    onBuild=[](){units[7].owner=2;};ai.Tick(1,s,1000,reserved);assert(!Touched(7) && Touched(8));
    Setup();ai.Reset();ai.Toggle();onBuild=[](){units[1].owner=2;};
    ai.Tick(1,s,1000,reserved);assert(orders.size()==1 && Touched(3) && !Touched(2) && !Touched(8));
    // One finished worker releases one production reservation; idle-order polling still cannot flood the queue.
    Setup();ai.Reset();ai.Toggle();s=ArmyAISettings();s.workers=3;s.soldiers=s.supplyBuildings=0;
    ai.Tick(1,s,1000,reserved);units[2].order=0;units[7].type=ArmyPlan::Code("hpea");orders.clear();
    ai.Tick(1,s,3000,reserved);assert(Touched(2));units[2].order=0;orders.clear();
    ai.Tick(1,s,5000,reserved);assert(!Touched(2));
    // Background/pause gaps do not consume the bounded retry lease; only accepted AI decisions advance it.
    ai.Tick(1,s,1000000000,reserved);assert(!Touched(2));
    // A campaign with only a hero/army has no legal economy: report it without granting a free worker.
    Setup();ai.Reset();ai.Toggle();s=ArmyAISettings();units[2].type=units[3].type=units[4].type=0;
    ai.Tick(1,s,1000,reserved);assert(ai.Status()==ArmyAIStatus::NoEconomy && builds==0);
    // Builders outrank captured foreign halls, and a missing hall is built near those workers rather than the hero.
    Setup();ai.Reset();ai.Toggle();units[3].type=ArmyPlan::Code("opeo");units[3].x=5000;
    ai.Tick(1,s,1000,reserved);assert(builds==1 && builtX>4000);
    assert(std::get<1>(orders.front())==ArmyPlan::Code("ogre"));
    // Bounded retries can find a legal site after several native rejects in the same decision.
    Setup();ai.Reset();ai.Toggle();rejectSites=3;ai.Tick(1,s,1000,reserved);
    assert(builds==4 && ai.Status()==ArmyAIStatus::Working);
    Setup();ai.Reset();ai.Toggle();buildAccepted=false;s.placementAttempts=5;ai.Tick(1,s,1000,reserved);assert(builds==5);
    // Undead acolytes haunt an available mine at its actual coordinates before expanding production.
    Setup();ai.Reset();ai.Toggle();s=ArmyAISettings();units[2].type=ArmyPlan::Code("unpl");units[3].type=ArmyPlan::Code("uaco");
    units[4].type=ArmyPlan::Code("usep");units[5].type=ArmyPlan::Code("ngol");units[5].x=-600;
    ai.Tick(1,s,1000,reserved);bool haunted=false;
    for(auto o:orders)haunted|=std::get<1>(o)==ArmyPlan::Code("ugol");
    assert(haunted && builtX==-600 && builtY==0);
    // The final undead campaign starts with workers but no base; idle ghouls must supply its missing wood.
    Setup();ai.Reset();ai.Toggle();units[2].type=0;units[3].type=ArmyPlan::Code("uaco");units[3].x=5000;
    units[4].type=0;units[7].type=units[8].type=units[9].type=ArmyPlan::Code("ugho");trees={900};
    ai.Tick(1,s,1000,reserved);int wood=0;
    for(auto o:orders)wood+=std::get<1>(o)==OrderId("harvest") && std::get<2>(o)==900;
    assert(wood==s.lumberWorkers && builtX>4000 && ai.Status()==ArmyAIStatus::Working);
    for(auto o:orders)if(std::get<1>(o)==OrderId("harvest") && std::get<2>(o)==900){units[std::get<0>(o)].order=OrderId("returnresources");break;}
    orders.clear();ai.Tick(1,s,3000,reserved);wood=0;
    for(auto o:orders)wood+=std::get<1>(o)==OrderId("harvest") && std::get<2>(o)==900;
    assert(wood==0); // Delivery is part of the same lumber job, never a reason to recruit the third ghoul.

    // Night elf halls use the native entangle ability; wisps never receive a fake raw-mine harvest.
    Setup();ai.Reset();ai.Toggle();units[2].type=ArmyPlan::Code("etol");units[2].entangle=true;
    units[3].type=ArmyPlan::Code("ewsp");units[4].type=ArmyPlan::Code("eaom");units[5].type=ArmyPlan::Code("ngol");units[5].x=-600;
    ai.Tick(1,s,1000,reserved);bool entangled=false;
    for(auto o:orders){entangled|=std::get<0>(o)==2 && std::get<1>(o)==OrderId("entangle") && std::get<2>(o)==5;
        assert(!(std::get<0>(o)==3 && std::get<1>(o)==OrderId("harvest") && std::get<2>(o)==5));}
    assert(entangled);
    // Native resource/tech rejection produces a useful blocker instead of a silent enabled assistant.
    Setup();ai.Reset();ai.Toggle();s=ArmyAISettings();buildAccepted=immediateAccepted=false;
    units[3].order=OrderId("harvest");units[5].type=ArmyPlan::Code("ngol");units[5].x=-600;
    ai.Tick(1,s,1000,reserved);assert(ai.Status()==ArmyAIStatus::Blocked);
    // Reaching targets while workers already harvest is healthy, rather than a false busy/no-resource warning.
    Setup();ai.Reset();ai.Toggle();s.workers=1;s.soldiers=s.supplyBuildings=0;units[3].order=OrderId("harvest");
    ai.Tick(1,s,1000,reserved);assert(ai.Status()==ArmyAIStatus::Ready);
    // Shop recognition also enforces distance, visibility/allegiance and living native stock providers.
    units[4].merchant=true;units[4].x=300;assert(ShopAccess::Nearby(1,4,450));
    units[4].x=451;assert(!ShopAccess::Nearby(1,4,450));units[4].x=300;units[4].owner=2;assert(!ShopAccess::Nearby(1,4,450));
    units[4].owner=1;units[4].hp=0;assert(!ShopAccess::Nearby(1,4,450));
    for(auto race:ArmyPlan::Races())assert(race.Complete() && race.Hall(race.hall));
    assert(ArmyPlan::Races()[0].Hall(ArmyPlan::Code("hcas")));
    return 0;
}
