#include "../src/platform/NativePortals.hpp"
#include "../src/platform/NativeRegistry.hpp"
#include "../src/platform/NativeExplosiveDeath.hpp"
#include "../src/platform/GameBuild.hpp"
#include <array>
#include <map>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>

namespace {
struct Unit {float x=0,y=0,hp=100;bool active=false,enemy=true,building=false;float toX=0,toY=0;unsigned identity=0;};
std::map<unsigned,Unit> units;
std::array<std::array<unsigned char,128>,32> objects{};
std::vector<unsigned> group;
float clockSeconds=0;int placements=0,trueFlags=0,falseFlags=0;bool valid=true;
std::function<void()> placed;
uint32_t Bits(float value){uint32_t bits=0;std::memcpy(&bits,&value,4);return bits;}
uint32_t& Flag(unsigned unit){return *reinterpret_cast<uint32_t*>(objects[unit+units[unit].identity].data()+0x5C);}
BOOL __cdecl Active(unsigned u){return units[u].active;}
uint32_t __cdecl DestinationX(unsigned u){return Bits(units[u].toX);}
uint32_t __cdecl DestinationY(unsigned u){return Bits(units[u].toY);}
void __cdecl Exploded(unsigned u,BOOL value){if(value){Flag(u)|=0x2000;++trueFlags;}else{Flag(u)&=~0x2000;++falseFlags;}}
void Scene(){units.clear();units[1]={-120,0,100,false,false};units[2]={0,0,100,true,true,false,1000,0};units[3]={1000,0,100,true,true,false,0,0};clockSeconds=0;placements=0;valid=true;placed={};objects={};trueFlags=falseFlags=0;}
}
namespace NativeRegistry {
uintptr_t Resolve(uintptr_t,const char* name,const char* signature) {
    if(!std::strcmp(name,"WaygateIsActive") && !std::strcmp(signature,"(Hunit;)B"))return reinterpret_cast<uintptr_t>(Active);
    if(!std::strcmp(name,"WaygateGetDestinationX") && !std::strcmp(signature,"(Hunit;)R"))return reinterpret_cast<uintptr_t>(DestinationX);
    if(!std::strcmp(name,"WaygateGetDestinationY") && !std::strcmp(signature,"(Hunit;)R"))return reinterpret_cast<uintptr_t>(DestinationY);
    return 0;
}
}
namespace wc3 {
#define NATIVE(result,name,arguments,offset) result (__cdecl* name) arguments=nullptr;
#include "../src/platform/NativeOffsets.inc"
#undef NATIVE
float Real(Bits bits){float value=0;std::memcpy(&value,&bits,4);return value;}
uintptr_t UnitObject(Handle unit){return units.count(unit) ? reinterpret_cast<uintptr_t>(objects[unit+units[unit].identity].data()) : 0;}
float Ground(float,float){return 0;}
void Log(const char*,...){}
}
int main() {
    _set_error_mode(_OUT_TO_STDERR);
    wc3::GetUnitTypeId=[](unsigned u){return units.count(u) ? 1 : 0;};
    wc3::GetUnitX=[](unsigned u){return Bits(units[u].x);};wc3::GetUnitY=[](unsigned u){return Bits(units[u].y);};
    wc3::GetUnitFlyHeight=[](unsigned){return Bits(0);};wc3::GetUnitState=[](unsigned u,int){return Bits(units[u].hp);};
    wc3::GetOwningPlayer=[](unsigned){return 1u;};wc3::IsUnitEnemy=[](unsigned u,unsigned)->BOOL{return units[u].enemy;};
    wc3::IsUnitType=[](unsigned u,int type)->BOOL{return type==2 && units[u].building;};
    wc3::IsUnitHidden=[](unsigned)->BOOL{return FALSE;};wc3::IsUnitLoaded=[](unsigned)->BOOL{return FALSE;};
    wc3::CreateTimer=[](){return 9u;};wc3::TimerStart=[](unsigned,float*,BOOL,unsigned){};
    wc3::TimerGetElapsed=[](unsigned){return Bits(clockSeconds);};
    wc3::CreateGroup=[](){group.clear();return 8u;};wc3::DestroyGroup=[](unsigned){};
    wc3::GroupEnumUnitsInRange=[](unsigned,float* x,float* y,float* radius,unsigned){for(const auto& p:units)if(std::hypot(p.second.x-*x,p.second.y-*y)<=*radius)group.push_back(p.first);};
    wc3::FirstOfGroup=[](unsigned){return group.empty() ? 0u : group.front();};
    wc3::GroupRemoveUnit=[](unsigned,unsigned u){group.erase(std::remove(group.begin(),group.end(),u),group.end());};
    wc3::GetWorldBounds=[](){return 7u;};wc3::RemoveRect=[](unsigned){};
    wc3::GetRectMinX=wc3::GetRectMinY=[](unsigned){return Bits(-4096);};wc3::GetRectMaxX=wc3::GetRectMaxY=[](unsigned){return Bits(4096);};
    wc3::SetUnitPosition=[](unsigned u,float* x,float* y){units[u].x=*x;units[u].y=*y;++placements;if(placed)placed();};
    Scene();NativePortals portals;portals.Configure(0);PortalSettings settings;
    auto alive=[](){return valid;};
    settings.enabled=false;assert(!portals.Travel(1,0,settings,true,alive));settings.enabled=true;
    assert(!portals.Travel(1,0,settings,false,alive));
    assert(!portals.Travel(1,500,settings,true,alive));clockSeconds+=.2f;
    assert(portals.Travel(1,0,settings,true,alive) && units[1].x==1000 && placements==1);
    clockSeconds=10;assert(!portals.Travel(1,0,settings,true,alive) && placements==1);
    units[1].x=1200;clockSeconds+=.2f;assert(!portals.Travel(1,0,settings,true,alive));
    units[1].x=1000;clockSeconds+=.2f;assert(portals.Travel(1,0,settings,true,alive) && units[1].x==0);
    Scene();portals.Reset();units[2].active=false;assert(!portals.Travel(1,0,settings,true,alive));
    units[2].active=true;units[2].toX=std::numeric_limits<float>::quiet_NaN();clockSeconds+=.2f;assert(!portals.Travel(1,0,settings,true,alive));
    units[2].toX=9000;clockSeconds+=.2f;assert(!portals.Travel(1,0,settings,true,alive) && !placements);
    Scene();portals.Reset();placed=[&]{valid=false;portals.Reset();};assert(!portals.Travel(1,0,settings,true,alive));
    Scene();portals.Reset();float displacement=-1;assert(!portals.Observe(1,48,&displacement) && displacement==0);units[1].x+=500;
    // The controller needs actual displacement, not just a teleport boolean, to retain local airborne motion.
    assert(portals.Observe(1,48,&displacement) && displacement==500);assert(!portals.Observe(1,48,&displacement) && displacement==0);
    units[1].x+=200;portals.Commit(1);assert(!portals.Observe(1,48)); // Our committed movement is never a script teleport.
    units[1].x+=100;assert(!portals.Observe(1,148)); // A budgeted fast motor substep is not a teleport either.
    // Scoped death flags survive neither a nonlethal hit nor handle reuse; native-dead targets retain their corpse policy.
    Scene();wc3::activeBuild=wc3::GameBuildKind::Classic127a;NativeExplosiveDeath::Configure(0);
    assert(!NativeExplosiveDeath::Available());wc3::activeBuild=wc3::GameBuildKind::Classic126a;
    NativeExplosiveDeath::TestBinding(Exploded,0x5C,0x2000);
    {NativeExplosiveDeath::Lease death(1,2,true);assert(Flag(2)&0x2000);}assert(!Flag(2) && trueFlags==1 && falseFlags==1);
    Flag(2)=0x2040;{NativeExplosiveDeath::Lease death(1,2,true);}assert(Flag(2)==0x2040 && trueFlags==1);
    Flag(2)=0;units[2].enemy=false;{NativeExplosiveDeath::Lease death(1,2,true);}assert(!Flag(2));units[2].enemy=true;
    units[2].building=true;{NativeExplosiveDeath::Lease death(1,2,true);}assert(!Flag(2));units[2].building=false;
    {NativeExplosiveDeath::Lease death(1,2,false);}assert(!Flag(2));
    {NativeExplosiveDeath::Lease death(1,2,true);units[2].hp=0;}assert(Flag(2)&0x2000);
    units[2].hp=100;Flag(2)=0;{NativeExplosiveDeath::Lease death(1,2,true);units[2].identity=10;}assert(!Flag(2));
    units[2].identity=0;Flag(2)=0;{NativeExplosiveDeath::Lease death(1,2,true);NativeExplosiveDeath::Reset();}assert(Flag(2)&0x2000);
    Flag(2)=0;{NativeExplosiveDeath::Lease death(1,2,true);Flag(2)=0;int count=falseFlags;death.Release();assert(falseFlags==count);}
    std::puts("PASS active portals, native destination/placement, vertical/bounds/disabled checks, no bounce, callbacks and scoped hostile creature deaths.");
}
