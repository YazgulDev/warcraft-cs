#include "../src/platform/NativeWeaponAttack.hpp"
#include "../src/platform/FpsCombatGuard.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <limits>

namespace {
std::array<unsigned char,0x300> actor{},victim{},attack{};
std::array<unsigned char,0x100> gates[2]{};int gateGeneration=0;bool gateAlive=true,reuseGateDuringPrepare=false;
uintptr_t expectedVictim=0,finishedTarget=0;
bool alive=true,removeDuringPrepare=false,removeDuringFinish=false,evade=false;
bool incomingProtected=false;
int prepared=0,finished=0,impacts=0;
int windups=0,poses=0;
float expectedRange=600;
int expectedKind=4;
wc3::Handle controlled=1;
using RollHook=float* (__fastcall*)(uintptr_t,uintptr_t,float*);
RollHook rollHook=nullptr;
using RangeHook=float* (__fastcall*)(uintptr_t,uintptr_t,float*,int);
RangeHook rangeHook=nullptr;
using ImpactHook=void (__fastcall*)(uintptr_t,uintptr_t,uintptr_t,int,int,int,int,int);
using PrepareHook=void (__fastcall*)(uintptr_t,uintptr_t,uintptr_t,float*,float*);
ImpactHook impactHook=nullptr;PrepareHook prepareHook=nullptr;
using WindupHook=void (__fastcall*)(uintptr_t,uintptr_t,int);
using PoseHook=void (__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
WindupHook windupHook=nullptr;PoseHook poseHook=nullptr;
void __fastcall Windup(uintptr_t,uintptr_t,int) {++windups;}
void __fastcall Pose(uintptr_t,uintptr_t,uintptr_t) {++poses;}
void __fastcall Prepare(uintptr_t,uintptr_t,uintptr_t,float*,float*);
void __fastcall Impact(uintptr_t,uintptr_t,uintptr_t,int,int,int,int,int);
uintptr_t Actor() {return reinterpret_cast<uintptr_t>(actor.data());}
uintptr_t Victim() {return reinterpret_cast<uintptr_t>(victim.data());}
uintptr_t Attack() {return reinterpret_cast<uintptr_t>(attack.data());}
float* __fastcall Roll(uintptr_t,uintptr_t,float* result) {*result=42;return result;}
float* __fastcall Range(uintptr_t,uintptr_t,float* result,int) {*result=120;return result;}
bool Hook(void* entry,void* callback,void** original) {
    auto bytes=static_cast<unsigned char*>(entry);
    if(bytes[0]==0x53){windupHook=reinterpret_cast<WindupHook>(callback);*original=reinterpret_cast<void*>(Windup);}
    else if(bytes[0]==0x83){poseHook=reinterpret_cast<PoseHook>(callback);*original=reinterpret_cast<void*>(Pose);}
    else if(bytes[0]==0x6A){impactHook=reinterpret_cast<ImpactHook>(callback);*original=reinterpret_cast<void*>(Impact);}
    else if(bytes[0]==0x51 && bytes[1]==0x57){prepareHook=reinterpret_cast<PrepareHook>(callback);*original=reinterpret_cast<void*>(Prepare);}
    else if(bytes[0]==0x51) {rollHook=reinterpret_cast<RollHook>(callback);*original=reinterpret_cast<void*>(Roll);}
    else {rangeHook=reinterpret_cast<RangeHook>(callback);*original=reinterpret_cast<void*>(Range);}
    return true;
}
void __fastcall Prepare(uintptr_t,uintptr_t,uintptr_t,float*,float*) {
    ++prepared;float result=0;
    // An attack callback may query another attack roll; it must not consume this contact's replacement.
    rollHook(Attack(),0,&result);assert(result==42);
    rangeHook(Attack(),0,&result,0);assert(result==120); // Preparation keeps native range.
    if(removeDuringPrepare)alive=false;
    if(reuseGateDuringPrepare)gateGeneration=1;
}
void __fastcall Finish(uintptr_t,uintptr_t,uintptr_t target) {++finished;finishedTarget=target;if(removeDuringFinish)alive=false;}
void __fastcall Impact(uintptr_t component,uintptr_t,uintptr_t target,int,int,int,int,int) {
    ++impacts;
    if(expectedVictim)assert(target==expectedVictim);
    assert(*reinterpret_cast<int*>(attack.data()+0xDC)==expectedKind);
    float range=0;
    rangeHook(component+4,0,&range,0);assert(range==120);
    rangeHook(component,0,&range,1);assert(range==120);
    rangeHook(component,0,&range,0);assert(range==expectedRange); // Only the confirmed primary contact extends range.
    if(evade)return;
    float result=0;rollHook(component+4,0,&result);assert(result==42);
    rollHook(component,0,&result);assert(result==123);
    rollHook(component,0,&result);assert(result==42); // No recursive/secondary hit receives the weapon base twice.
}
void Jump(uintptr_t address,void* destination) {
    *reinterpret_cast<unsigned char*>(address)=0xE9;
    *reinterpret_cast<int*>(address+1)=int(reinterpret_cast<uintptr_t>(destination)-address-5);
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),5);
}
}
namespace wc3 {
uintptr_t UnitObject(Handle handle) {return handle==1 && alive ? Actor() : handle==2 ? Victim() : 0;}
uintptr_t DestructableObject(Handle handle) {return handle==3 && gateAlive ? uintptr_t(gates[gateGeneration].data()) : 0;}
int (__cdecl* GetUnitTypeId)(Handle)=[](Handle h){return UnitObject(h) ? 123 : 0;};
int (__cdecl* GetDestructableTypeId)(Handle)=[](Handle h){return DestructableObject(h) ? 456 : 0;};
void Log(const char*,...) {}
void Trace(const char*,...) {}
}
namespace FpsCombatGuard {
bool BlocksIncomingAttack(uintptr_t,uintptr_t) {return incomingProtected;}
}
int main() {
    auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x282000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(memory);
    uintptr_t base=reinterpret_cast<uintptr_t>(memory);
    *reinterpret_cast<uintptr_t*>(actor.data()+0x1E8)=Attack();
    *reinterpret_cast<int*>(attack.data()+0xDC)=2;
    // Direct damage keeps the unit's native armor class even when passive dispatch is disabled.
    *reinterpret_cast<int*>(attack.data()+0xF4)=6;assert(NativeWeaponAttack::AttackType(1)==6);
    *reinterpret_cast<int*>(attack.data()+0xF4)=99;assert(NativeWeaponAttack::AttackType(1)==0);
    *reinterpret_cast<int*>(attack.data()+0xF4)=6;
    assert(!NativeWeaponAttack::Apply(1,2,123,600));
    // Layout mismatch must disable the adapter, preserving the caller's ordinary direct-damage fallback.
    NativeWeaponAttack::Install(base,Hook);assert(!NativeWeaponAttack::Apply(1,2,123,600));
    const unsigned char impact[]={0x6A,0xFF,0x68},roll[]={0x51,0x56,0x57,0x68},cleanup[]={0xC2,0x18,0};
    const unsigned char prepare[]={0x51,0x57,0x8B,0xF9},finish[]={0x8B,0x81,0xE0,1,0,0};
    memcpy(memory+0xCF660,impact,sizeof(impact));memcpy(memory+0xC6940,roll,sizeof(roll));
    memcpy(memory+0xD0225,cleanup,sizeof(cleanup));memcpy(memory+0x281580,prepare,sizeof(prepare));memcpy(memory+0x276330,finish,sizeof(finish));
    const unsigned char range[]={0x56,0x8B,0xF1,0x8B,0x86,0xC0,2,0,0};memcpy(memory+0xC6B30,range,sizeof(range));
    const unsigned char windup[]={0x53,0x55,0x56,0x8B,0xF1,0x8B,0x46,0x70},pose[]={0x83,0xEC,8,0x56,0x57,0x8B,0xF9};
    memcpy(memory+0xCF5C0,windup,sizeof(windup));memcpy(memory+0xC8930,pose,sizeof(pose));
    *reinterpret_cast<int*>(memory+0xCF82B)=0xC6940-0xCF82F;
    NativeWeaponAttack::Install(base,Hook,[](){return controlled;},[](uintptr_t unit){return unit==Victim();},[](uintptr_t component){return component==777;});
    // The mock executable entries route through the real guards, matching the installed detours' argument layouts.
    Jump(base+0xCF660,reinterpret_cast<void*>(impactHook));Jump(base+0x281580,reinterpret_cast<void*>(prepareHook));Jump(base+0x276330,reinterpret_cast<void*>(Finish));
    float zero=0;prepareHook(Actor(),0,Victim(),&zero,&zero);impactHook(Attack(),0,Victim(),0,0,1,0,1);
    prepareHook(Victim(),0,Actor(),&zero,&zero);impactHook(777,0,Actor(),0,0,1,0,1);
    assert(!prepared && !impacts); // Retained autonomous targets cannot prepare effects or damage.
    windupHook(Attack(),0,0);poseHook(Attack(),0,Actor());windupHook(777,0,0);poseHook(777,0,Victim());
    assert(!windups && !poses); // Native retry timers and ranged attack poses are blocked too, not just damage.
    windupHook(888,0,0);poseHook(888,0,Actor());assert(windups==1 && poses==1);
    assert(NativeWeaponAttack::PrimaryKind(1)==2 && NativeWeaponAttack::PrimaryRange(1)==120);
    assert(!NativeWeaponAttack::Apply(0,2,123,600) && !NativeWeaponAttack::Apply(1,0,123,600));
    assert(!NativeWeaponAttack::Apply(1,2,0,600) && !NativeWeaponAttack::Apply(1,2,std::numeric_limits<float>::quiet_NaN(),600));
    assert(!NativeWeaponAttack::Apply(1,2,123,-1) && !NativeWeaponAttack::Apply(1,2,123,std::numeric_limits<float>::infinity()));
    assert(NativeWeaponAttack::Apply(1,2,123,600) && prepared==1 && impacts==1 && finished==1);
    assert(*reinterpret_cast<int*>(attack.data()+0xDC)==2);
    float ordinary=0;rollHook(Attack(),0,&ordinary);assert(ordinary==42);
    rangeHook(Attack(),0,&ordinary,0);assert(ordinary==120); // No permanent RTS range change.
    // A native miss has already dispatched effects: returning handled prevents a second fallback damage event.
    evade=true;assert(NativeWeaponAttack::Apply(1,2,123,600) && impacts==2 && finished==2);
    assert(*reinterpret_cast<int*>(attack.data()+0xDC)==2);
    // Cleanup callbacks may also remove the actor; their former component must not be restored afterwards.
    evade=false;removeDuringFinish=true;assert(NativeWeaponAttack::Apply(1,2,123,600));
    assert(impacts==3 && finished==3 && *reinterpret_cast<int*>(attack.data()+0xDC)==4);
    alive=true;removeDuringFinish=false;*reinterpret_cast<int*>(attack.data()+0xDC)=2;
    // Transformation/removal during attack callbacks cancels impact and never writes the former component.
    evade=false;removeDuringPrepare=true;assert(NativeWeaponAttack::Apply(1,2,123,600));
    assert(impacts==3 && finished==3);
    alive=true;removeDuringPrepare=false;*reinterpret_cast<int*>(attack.data()+0xDC)=2;expectedRange=120;
    // Nearby contacts keep the larger native range rather than shrinking it to target distance.
    assert(NativeWeaponAttack::Apply(1,2,123,60) && impacts==4 && finished==4);
    rangeHook(Attack(),0,&ordinary,0);assert(ordinary==120);
    // Ranged launch snapshots the scoped damage while retaining missile kind and restoring ordinary stats afterwards.
    expectedKind=2;assert(NativeWeaponAttack::Launch(1,2,123,60) && impacts==5 && finished==5);
    assert(*reinterpret_cast<int*>(attack.data()+0xDC)==2);
    // Destructable missiles use the exact widget through prepare/impact/finish, without a unit cast or double hit.
    expectedVictim=wc3::DestructableObject(3);
    assert(NativeWeaponAttack::Launch(1,3,123,60) && impacts==6 && finished==6 && finishedTarget==expectedVictim);
    assert(*reinterpret_cast<int*>(attack.data()+0xDC)==2);
    reuseGateDuringPrepare=true;
    assert(NativeWeaponAttack::Launch(1,3,123,60) && impacts==6 && finished==7 && finishedTarget==0);
    reuseGateDuringPrepare=false;gateAlive=false;
    assert(!NativeWeaponAttack::Launch(1,3,123,60) && impacts==6 && finished==7);
    expectedVictim=0;
    int previousPrepared=prepared;controlled=0;
    // Passive-view protection must reject both preparation and impact before native callbacks run.
    incomingProtected=true;
    prepareHook(Actor(),0,Victim(),&zero,&zero);impactHook(Attack(),0,Victim(),0,0,1,0,1);
    assert(prepared==previousPrepared && impacts==6);
    incomingProtected=false;prepareHook(Actor(),0,Victim(),&zero,&zero);
    assert(prepared==previousPrepared+1); // Returning to RTS restores autonomous wind-up callbacks.
    windupHook(Attack(),0,0);poseHook(Attack(),0,Actor());assert(windups==2 && poses==2);
    VirtualFree(memory,0,MEM_RELEASE);
}
