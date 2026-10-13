#include "../src/platform/UnitAttack.hpp"
#include "../src/platform/NativeWeaponAttack.hpp"
#include "../src/platform/WeaponTargetFilter.hpp"
#include <cassert>
#include <cstring>

namespace {
float targetX=250,reach=600,health=420;
bool hidden=false,loaded=false,visible=true,transparent=false,wall=false,ally=false;
int animations=0,damageEvents=0,kind=2;
std::vector<wc3::Handle> group;
wc3::Bits Pack(float x){wc3::Bits value;std::memcpy(&value,&x,4);return value;}
}
namespace wc3 {
#define NATIVE(result,name,arguments,offset) result (__cdecl* name) arguments=nullptr;
#include "../src/platform/NativeOffsets.inc"
#undef NATIVE
float Real(Bits x){float value;std::memcpy(&value,&x,4);return value;}
void Log(const char*,...){}
void Animate(Handle,const char*){++animations;}
// The real manual pose adapter can run without retail models, using the contact-duration fallback.
std::string UnitModelPath(int){return "synthetic.mdx";}
int AnimationSequence(Handle){return -1;}
void AnimateIndex(Handle,int){++animations;}
float AnimationRate(Handle){return 1;}
void AnimationRate(Handle,float){}
void AnimationSeek(Handle,int,float){}
float Ground(float x,float){return wall && x>=120 ? 200.f : 0.f;}
std::vector<Handle> NearbyDestructables(float,float,float){return {};}
float UnitAttackAverage(Handle){return 100;}
}
namespace NativeWeaponAttack {
int PrimaryKind(wc3::Handle){return kind;}
float PrimaryRange(wc3::Handle){return reach;}
int AttackType(wc3::Handle){return 0;}
bool Apply(wc3::Handle,wc3::Handle,float,float){return false;}
bool Launch(wc3::Handle,wc3::Handle,float,float){return false;}
}
bool WeaponTargetFilter::PassThrough(wc3::Handle,uintptr_t){return transparent;}
bool UnitHitboxes::Intersect(wc3::Handle,const float* origin,const float* direction,float limit,float& entry,float) {
    Bounds3 box={{targetX-20,-20,0},{targetX+20,20,128}};
    return IntersectBounds(box,origin,direction,limit,entry);
}
bool DestructableHitboxes::Intersect(wc3::Handle,const float*,const float*,float,float&){return false;}
int main(){
    using namespace wc3;
    GetUnitTypeId=[](Handle){return 1;};
    CreateGroup=[]()->Handle{return 1;};DestroyGroup=[](Handle){};
    GroupEnumUnitsInRange=[](Handle,float*,float*,float*,Handle){group={2};};
    FirstOfGroup=[](Handle)->Handle{return group.empty() ? 0 : group.back();};
    GroupRemoveUnit=[](Handle,Handle){group.pop_back();};
    GetUnitX=[](Handle unit){return Pack(unit==1 ? 0 : targetX);};GetUnitY=[](Handle){return Pack(0);};
    GetUnitState=[](Handle,int){return Pack(health);};
    IsUnitHidden=[](Handle)->BOOL{return hidden;};IsUnitLoaded=[](Handle)->BOOL{return loaded;};
    IsUnitVisible=[](Handle,Handle)->BOOL{return visible;};GetLocalPlayer=[]()->Handle{return 1;};
    GetOwningPlayer=[](Handle)->Handle{return 1;};IsUnitAlly=[](Handle,Handle)->BOOL{return ally;};
    IsUnitEnemy=[](Handle,Handle)->BOOL{return !ally;};
    UnitDamageTarget=[](Handle,Handle,float* amount,BOOL,BOOL,int,int,int)->BOOL{++damageEvents;health-=*amount;return TRUE;};
    UnitAttack attack;GameplaySettings settings;UnitHitboxes units;DestructableHitboxes objects;
    float origin[]={-500,0,64},direction[]={1,0,0};
    auto preview=[&]{return attack.Preview(1,0,0,origin,direction,settings,units,objects);};
    // The real picker predicts native reach and parallax while generating no attack/animation/damage event.
    auto picked=preview();assert(picked.handle==2 && !picked.destructable && picked.position[0]>=229 && picked.position[0]<=231);
    assert(animations==0 && damageEvents==0 && health==420);
    assert(picked.enemy && picked.circle[0][0]==314 && picked.circle[0][2]==3);
    settings.thirdPerson.targetCircleRadius=96;assert(preview().circle[0][0]==346);
    settings.thirdPerson.targetCircleRadius=64;
    reach=100;assert(!preview().handle);reach=600;
    targetX=1500;assert(!preview().handle);targetX=250;
    wall=true;assert(!preview().handle);wall=false;
    hidden=true;assert(!preview().handle);hidden=false;
    loaded=true;assert(!preview().handle);loaded=false;
    visible=false;assert(!preview().handle);visible=true;
    transparent=true;assert(!preview().handle);transparent=false;
    health=0;assert(!preview().handle);health=420;
    ally=true;assert(preview().allied && !preview().enemy);ally=false;
    settings.thirdPerson.useNativeAttackRange=false;settings.thirdPerson.rangedAttackRange=100;assert(!preview().handle);
    settings.thirdPerson.rangedAttackRange=600;
    // A delayed manual contact selects the same body the idle preview marked, preserving zero idle damage.
    UnitStatus status;assert(attack.Begin(1,0,settings,status));
    assert(attack.Tick(1,1,0,0,origin,direction,false,settings,status,units,objects));
    assert(damageEvents==1 && health==320);
    kind=1;targetX=100;assert(preview().handle==2);targetX=250;assert(!preview().handle);
    kind=0;assert(!preview().handle);
}
