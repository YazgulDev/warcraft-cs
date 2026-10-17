#include "../src/platform/FpsCombatGuard.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
uintptr_t base=0,actorAttack=11,passiveAttack=12;
wc3::Handle actor=1;
alignas(uintptr_t) std::array<unsigned char,0x200> actorObject={};
alignas(uintptr_t) std::array<unsigned char,0x200> otherObject={};
int checks=0,starts=0,stops=0;
std::array<uintptr_t,10> arguments={};
using Check=int (__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
using Start=void (__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
Check checked=nullptr;
Start started=nullptr;
uintptr_t (__fastcall* targetGetter)(uintptr_t,uintptr_t)=nullptr;
uintptr_t __fastcall NativeTargetGetter(uintptr_t,uintptr_t) {return 123;}
BOOL (__cdecl* targetOrder)(wc3::Handle,int,wc3::Handle)=nullptr;
BOOL (__cdecl* pointOrder)(wc3::Handle,int,float*,float*)=nullptr;
int scriptOrders=0;
BOOL __cdecl NativeTarget(wc3::Handle,int,wc3::Handle) {++scriptOrders;return TRUE;}
BOOL __cdecl NativePoint(wc3::Handle,int,float* x,float*) {++scriptOrders;arguments[0]=uintptr_t(*x);return TRUE;}
int __fastcall NativeCheck(uintptr_t,uintptr_t,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d) {
    ++checks;arguments={a,b,c,d};return 17;
}
void __fastcall NativeStart(uintptr_t,uintptr_t,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,
    uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j) {
    ++starts;arguments={a,b,c,d,e,f,g,h,i,j};
}
float* __fastcall NativeRange(uintptr_t,uintptr_t,float* result) {*result=500;return result;}
bool Hook(void* at,void* detour,void** original) {
    if(uintptr_t(at)==base+0x7C2C0) {targetGetter=reinterpret_cast<decltype(targetGetter)>(detour);*original=reinterpret_cast<void*>(NativeTargetGetter);}
    else if(uintptr_t(at)==base+0xC5E30) {checked=reinterpret_cast<Check>(detour);*original=reinterpret_cast<void*>(NativeCheck);}
    else if(uintptr_t(at)==base+0xC6210) {started=reinterpret_cast<Start>(detour);*original=reinterpret_cast<void*>(NativeStart);}
    else if(at==reinterpret_cast<void*>(wc3::IssueTargetOrderById)) {targetOrder=reinterpret_cast<decltype(targetOrder)>(detour);*original=reinterpret_cast<void*>(NativeTarget);}
    else if(at==reinterpret_cast<void*>(wc3::IssuePointOrderById)) {pointOrder=reinterpret_cast<decltype(pointOrder)>(detour);*original=reinterpret_cast<void*>(NativePoint);}
    else *original=reinterpret_cast<void*>(NativeRange);
    return true;
}
}
namespace wc3 {
void Log(const char*,...) {}
void Trace(const char*,...) {}
uintptr_t UnitObject(Handle unit) {return unit==1 ? uintptr_t(actorObject.data()) : unit==2 ? uintptr_t(otherObject.data()) : 0;}
int (__cdecl* GetUnitCurrentOrder)(Handle)=[](Handle){return 0;};
BOOL (__cdecl* IssueImmediateOrderById)(Handle,int)=[](Handle,int)->BOOL{++stops;return TRUE;};
BOOL (__cdecl* IssueTargetOrderById)(Handle,int,Handle)=NativeTarget;
BOOL (__cdecl* IssuePointOrderById)(Handle,int,float*,float*)=NativePoint;
// Synthetic ownership keeps the incoming-attack policy independent of Warcraft's player table.
Handle (__cdecl* Player)(int)=[](int index)->Handle{return Handle(index+10);};
Handle (__cdecl* GetOwningPlayer)(Handle)=[](Handle unit)->Handle{return unit==1 ? 10 : 11;};
BOOL (__cdecl* IsUnitEnemy)(Handle,Handle)=[](Handle unit,Handle owner)->BOOL{return unit==1 && owner==11;};
}
int main() {
    // Read-only identity fails closed until the native target signature is verified.
    assert(FpsCombatGuard::AttackTargetObject(1)==0);
    std::vector<unsigned char> image(0x3D0000);base=uintptr_t(image.data());
    auto actorGetter=[](){return actor;};auto unitPolicy=[](uintptr_t){return false;};
    auto attackPolicy=[](uintptr_t at){return at==passiveAttack;};
    // A wrong host signature must not install new dispatch hooks.
    FpsCombatGuard::Install(base,actorGetter,unitPolicy,attackPolicy,Hook);assert(!checked && !started);
    const unsigned char target[]={0x8B,0x41,0x70,0x23,0x41,0x6C};
    const unsigned char check[]={0x56,0x8B,0xF1,0x8B,0x06,0x8B,0x90,0xA0,0,0,0};
    const unsigned char start[]={0x56,0x8B,0xF1,0x83,0x7E,0x40,0};
    std::memcpy(image.data()+0x7C2C0,target,sizeof(target));std::memcpy(image.data()+0xC5E30,check,sizeof(check));
    std::memcpy(image.data()+0xC6210,start,sizeof(start));std::memcpy(actorObject.data()+0x1E8,&actorAttack,sizeof(actorAttack));
    const unsigned char numeric[]={0x8B,0x4C,0x24,0x04},text[]={0x8B,0x4C,0x24,0x08,0xE8},convert[]={0x56,0x8B,0x35};
    for(auto at:{0x3C89D0,0x3C8940})std::memcpy(image.data()+at,numeric,sizeof(numeric));
    for(auto at:{0x3C8E20,0x3C8DA0})std::memcpy(image.data()+at,text,sizeof(text));
    std::memcpy(image.data()+0x3BAD70,convert,sizeof(convert));
    FpsCombatGuard::Install(base,actorGetter,unitPolicy,attackPolicy,Hook);assert(checked && started);
    assert(FpsCombatGuard::AttackTargetObject(1)==123 && FpsCombatGuard::HasAttackTarget(1));
    // Native morph/combat callbacks require a non-null retained target; pursuit is blocked at dispatch/motor.
    assert(targetGetter && targetGetter(actorAttack,0)==123 && targetGetter(passiveAttack,0)==123);
    assert(targetGetter(13,0)==123 && FpsCombatGuard::HasAttackTarget(1));
    // One Stop per acquired target; squad observations do not restart the controlled hero's manual animation.
    *reinterpret_cast<uintptr_t*>(otherObject.data()+0x1E8)=13;
    FpsCombatGuard::CancelOrders(1);FpsCombatGuard::CancelOrders(2);FpsCombatGuard::CancelOrders(1);FpsCombatGuard::CancelOrders(2);
    assert(stops==2);
    // Neither validation nor direct retaliation dispatch can start the actor's basic attack.
    assert(checked(actorAttack,0,1,2,3,4)==0xDD);started(actorAttack,0,1,2,3,4,5,6,7,8,9,10);
    assert(!checks && !starts);
    assert(checked(passiveAttack,0,1,2,3,4)==0xDD);started(passiveAttack,0,1,2,3,4,5,6,7,8,9,10);
    assert(!checks && !starts);
    // Spell components and other units retain their original arguments and native result.
    assert(checked(13,0,1,2,3,4)==17 && checks==1 && arguments[3]==4);
    started(13,0,1,2,3,4,5,6,7,8,9,10);assert(starts==1 && arguments[9]==10);
    // Map orders cannot sneak in a chase step; spell targets and smart item pickup still reach Warcraft.
    assert(targetOrder && pointOrder);float x=10,y=20;
    assert(!targetOrder(1,851983,2) && !targetOrder(1,851971,2) && !pointOrder(1,851983,&x,&y));
    assert(scriptOrders==0 && targetOrder(1,852089,2) && targetOrder(1,851971,3));
    assert(pointOrder(1,852089,&x,&y) && targetOrder(2,851983,1) && scriptOrders==4);
    // F6/cinematic release returns the same attack component to the native engine immediately.
    actor=0;assert(checked(actorAttack,0,1,2,3,4)==17 && checks==2);
    started(actorAttack,0,1,2,3,4,5,6,7,8,9,10);assert(starts==2);
    assert(targetOrder(1,851983,2) && pointOrder(1,851983,&x,&y) && scriptOrders==6);
    std::puts("PASS basic attack validation/retaliation blocked in FPS; spells, other units and RTS dispatch preserved.");
}
