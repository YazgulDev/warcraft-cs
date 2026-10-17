#include "../src/platform/UnitAttack.hpp"
#include "../src/platform/NativeWeaponAttack.hpp"
#include "../src/platform/WeaponTargetFilter.hpp"
#include "../src/platform/AttackTargetPolicy.hpp"
#include <cassert>
#include <cstring>

namespace {
float targetX=250,reach=600,health=420;
bool hidden=false,loaded=false,visible=true,transparent=false,wall=false,ally=false,water=false,neutral=false;
bool screen=false,reverse=false,screenAllied=true,gate=false;
float screenX=60,screenHealth=500;
int animations=0,damageEvents=0,kind=2;
bool launchHandled=false;int launches=0;wc3::Handle launchedTarget=0;
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
int ActiveAnimationSequence(Handle){return -1;}
void AnimateIndex(Handle,int){++animations;}
float AnimationRate(Handle){return 1;}
void AnimationRate(Handle,float){}
void AnimationSeek(Handle,int,float){}
bool blending=true;
bool AnimationBlending(Handle){return blending;}
void AnimationBlending(Handle,bool enabled){blending=enabled;}
float Ground(float x,float){return wall && x>=120 ? 200.f : 0.f;}
// A synthetic high liquid plane reproduces the old false wall while physical terrain/decks remain solid.
float AttackSurface(float x,float y,bool blocksWater){return water && blocksWater && x>=40 ? 160.f : Ground(x,y);}
std::vector<Handle> NearbyDestructables(float,float,float){return gate ? std::vector<Handle>{4} : std::vector<Handle>{};}
float UnitAttackAverage(Handle){return 100;}
bool IsNeutralUnit(Handle unit){return unit==2 && neutral;}
}
namespace NativeWeaponAttack {
int PrimaryKind(wc3::Handle){return kind;}
float PrimaryRange(wc3::Handle){return reach;}
int AttackType(wc3::Handle){return 0;}
bool Apply(wc3::Handle,wc3::Handle,float,float){return false;}
bool Launch(wc3::Handle,wc3::Handle target,float,float){++launches;launchedTarget=target;return launchHandled;}
}
bool WeaponTargetFilter::PassThrough(wc3::Handle,uintptr_t){return transparent;}
bool UnitHitboxes::Intersect(wc3::Handle unit,const float* origin,const float* direction,float limit,float& entry,float) {
    float x=unit==3 ? screenX : targetX;
    Bounds3 box={{x-20,-20,0},{x+20,20,128}};
    return IntersectBounds(box,origin,direction,limit,entry);
}
bool DestructableHitboxes::Intersect(wc3::Handle,const float* origin,const float* direction,float limit,float& entry){
    return gate && IntersectBounds({{70,-100,0},{75,100,200}},origin,direction,limit,entry);
}
int main(){
    using namespace wc3;
    GetUnitTypeId=[](Handle){return 1;};
    CreateGroup=[]()->Handle{return 1;};DestroyGroup=[](Handle){};
    GroupEnumUnitsInRange=[](Handle,float*,float*,float*,Handle){group=screen ? (reverse ? std::vector<Handle>{2,3} : std::vector<Handle>{3,2}) : std::vector<Handle>{2};};
    FirstOfGroup=[](Handle)->Handle{return group.empty() ? 0 : group.back();};
    GroupRemoveUnit=[](Handle,Handle){group.pop_back();};
    GetUnitX=[](Handle unit){return Pack(unit==1 ? 0 : unit==3 ? screenX : targetX);};GetUnitY=[](Handle){return Pack(0);};
    GetUnitState=[](Handle unit,int){return Pack(unit==3 ? screenHealth : health);};
    GetDestructableLife=[](Handle){return Pack(500);};
    GetDestructableX=[](Handle){return Pack(72.5f);};GetDestructableY=[](Handle){return Pack(0);};
    IsUnitHidden=[](Handle)->BOOL{return hidden;};IsUnitLoaded=[](Handle)->BOOL{return loaded;};
    IsUnitVisible=[](Handle,Handle)->BOOL{return visible;};GetLocalPlayer=[]()->Handle{return 1;};
    GetOwningPlayer=[](Handle)->Handle{return 1;};IsUnitAlly=[](Handle unit,Handle)->BOOL{return unit==3 ? screenAllied : ally;};
    IsUnitEnemy=[](Handle unit,Handle)->BOOL{return !(unit==3 ? screenAllied : ally);};
    UnitDamageTarget=[](Handle,Handle target,float* amount,BOOL,BOOL,int,int,int)->BOOL{++damageEvents;(target==3 ? screenHealth : health)-=*amount;return TRUE;};
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
    water=true;assert(preview().handle==2);settings.waterBlocksAttacks=true;assert(!preview().handle);
    settings.waterBlocksAttacks=false;assert(preview().handle==2);
    wall=true;assert(!preview().handle);wall=false;water=false;
    hidden=true;assert(!preview().handle);hidden=false;
    loaded=true;assert(!preview().handle);loaded=false;
    visible=false;assert(!preview().handle);visible=true;
    transparent=true;assert(!preview().handle);transparent=false;
    health=0;assert(!preview().handle);health=420;
    // Allied contacts are opt-in; the exact same prediction/contact policy is used for melee and ranged attacks.
    ally=true;assert(!preview().handle);settings.targetAllies=true;assert(preview().allied && !preview().enemy);
    settings.targetAllies=false;ally=false;
    settings.thirdPerson.useNativeAttackRange=false;settings.thirdPerson.rangedAttackRange=100;assert(!preview().handle);
    settings.thirdPerson.rangedAttackRange=600;
    // A delayed manual contact selects the same body the idle preview marked, preserving zero idle damage.
    UnitStatus status;assert(attack.Begin(1,0,settings,status));
    assert(!blending); // Pose repairs must not restart native interpolation every movement frame.
    assert(attack.Tick(1,1,0,0,origin,direction,false,settings,status,units,objects));
    assert(damageEvents==1 && health==320 && blending); // Completion restores the exact native policy.
    // Contacts at impact respect a toggle changed during wind-up, rather than a cached friendly prediction.
    attack.Reset();health=420;ally=true;assert(attack.Begin(1,2,settings,status));
    assert(!attack.Tick(1,3,0,0,origin,direction,false,settings,status,units,objects));assert(damageEvents==1 && health==420);
    attack.Reset();settings.targetAllies=true;assert(attack.Begin(1,4,settings,status));
    assert(attack.Tick(1,5,0,0,origin,direction,false,settings,status,units,objects));assert(damageEvents==2 && health==370);
    settings.targetAllies=false;ally=false;
    kind=1;targetX=100;assert(preview().handle==2);targetX=250;assert(!preview().handle);
    // Melee arcs use the same water setting; toggling liquid transparency never removes real terrain occlusion.
    targetX=100;water=true;assert(preview().handle==2);settings.waterBlocksAttacks=true;assert(!preview().handle);
    settings.waterBlocksAttacks=false;assert(preview().handle==2);water=false;targetX=250;
    targetX=100;ally=true;assert(!preview().handle);settings.targetAllies=true;assert(preview().allied);
    // Protected front allies must not absorb selection. Check both enumeration orders and both attack kinds,
    // including actual delayed damage, Q-style toggles, reach and gates/terrain that remain solid.
    screen=true;ally=false;settings.targetAllies=false;
    double now=10;
    for(int weaponKind:{1,2})for(bool order:{false,true}) {
        kind=weaponKind;reverse=order;targetX=kind==1 ? 100.f : 250.f;health=420;screenHealth=500;
        assert(preview().handle==2 && preview().enemy);
        attack.Reset();assert(attack.Begin(1,now,settings,status));
        assert(attack.Tick(1,now+1,0,0,origin,direction,false,settings,status,units,objects));now+=2;
        assert(health==320 && screenHealth==500);
        settings.targetAllies=true;assert(preview().handle==3 && preview().allied);
        attack.Reset();assert(attack.Begin(1,now,settings,status));
        assert(attack.Tick(1,now+1,0,0,origin,direction,false,settings,status,units,objects));now+=2;
        assert(health==320 && screenHealth==450);
        settings.targetAllies=false;assert(preview().handle==2);
        gate=true;assert(preview().handle==4 && preview().destructable);gate=false;
        if(kind==2){wall=true;assert(!preview().handle);wall=false;reach=100;settings.thirdPerson.useNativeAttackRange=true;assert(!preview().handle);reach=600;}
        screenAllied=false;assert(preview().handle==3 && preview().enemy);screenAllied=true;
    }
    // No enemy in reach means a miss rather than an accidental allied contact.
    targetX=1500;assert(!preview().handle);settings.targetAllies=true;assert(preview().handle==3);
    kind=0;assert(!preview().handle);
    // Neutral-passive structures can carry an allied bit; neither selection nor contact may apply friendly protection.
    screen=false;ally=true;neutral=true;health=420;settings.targetAllies=false;
    assert(!AttackTargetPolicy::FinishingAllowed(2,1));
    for(int weaponKind:{1,2}) {
        kind=weaponKind;targetX=kind==1 ? 100.f : 250.f;health=420;
        assert(preview().handle==2 && !preview().allied && !preview().enemy);
        attack.Reset();assert(attack.Begin(1,now,settings,status));
        assert(attack.Tick(1,now+1,0,0,origin,direction,false,settings,status,units,objects));now+=2;
        assert(health==320);
    }
    // Ranged gates dispatch a native missile rather than an extra instant damage event; opt-outs retain direct contact.
    gate=true;ally=false;kind=2;targetX=250;launchHandled=true;
    int priorLaunches=launches,priorDamage=damageEvents;
    attack.Reset();assert(attack.Begin(1,now,settings,status));
    assert(!attack.Tick(1,now+1,0,0,origin,direction,false,settings,status,units,objects));now+=3;
    assert(launches==priorLaunches+1 && launchedTarget==4 && damageEvents==priorDamage);
    for(int fallback=0;fallback<4;++fallback) {
        launchHandled=fallback!=0;settings.thirdPerson.nativeRangedProjectiles=fallback!=1;
        settings.enableAttackEffects=fallback!=2;kind=fallback==3 ? 1 : 2;
        priorLaunches=launches;priorDamage=damageEvents;
        attack.Reset();assert(attack.Begin(1,now,settings,status));
        attack.Tick(1,now+1,0,0,origin,direction,false,settings,status,units,objects);now+=3;
        assert(damageEvents==priorDamage+1 && launches==priorLaunches+(fallback==0 ? 1 : 0));
    }
}
