#include "FpsCombatGuard.hpp"
#include "../combat/NativeOrderCancellation.hpp"
#include <cstring>

namespace {
uintptr_t gameBase=0;
bool targetVerified=false;
wc3::Handle (*viewActor)()=nullptr;
wc3::Handle (*protectedActor)()=nullptr;
bool eligibilityVerified=false;
bool (*passiveUnit)(uintptr_t)=nullptr,(*passiveAttack)(uintptr_t)=nullptr;
uintptr_t ActorObject() {
    wc3::Handle actor=viewActor ? viewActor() : 0;
    return wc3::UnitObject(actor);
}
bool ControlledAttack(uintptr_t attack) {
    uintptr_t actor=ActorObject();return actor && (*reinterpret_cast<uintptr_t*>(actor+0x1E8)==attack || (passiveAttack && passiveAttack(attack)));
}
using Range=float* (__thiscall*)(uintptr_t,float*);
Range unitRange=nullptr,attackRange=nullptr;
using Target=uintptr_t (__thiscall*)(uintptr_t);
Target nativeTarget=nullptr;
using Eligibility=int (__thiscall*)(uintptr_t,uintptr_t,int*);
Eligibility nativeEligibility=nullptr;
int __fastcall EligibilityHook(uintptr_t attack,uintptr_t,uintptr_t target,int* index) {
    // Warcraft's shared target test covers acquisition and retained windups, before chase/attack orders.
    if(FpsCombatGuard::BlocksIncomingAttack(attack,target)) {
        if(index)*index=-1;
        return 0xDD; // Same native no-valid-attack rejection returned by this verified routine.
    }
    return nativeEligibility(attack,target,index);
}
uintptr_t __fastcall AttackTargetHook(uintptr_t attack,uintptr_t) {
    // Morph/combat callbacks dereference an existing target without checking null (1.26a +0xCD86D).
    // Preserve that native contract; dispatch guards and the separate walking motor prevent pursuit.
    return nativeTarget(attack);
}
using CheckOrder=int (__thiscall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
using StartOrder=void (__thiscall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
CheckOrder checkOrder=nullptr;
StartOrder startOrder=nullptr;
using TargetOrder=BOOL (__cdecl*)(wc3::Handle,int,wc3::Handle);
using PointOrder=BOOL (__cdecl*)(wc3::Handle,int,float*,float*);
TargetOrder targetId=nullptr,targetString=nullptr;
PointOrder pointId=nullptr,pointString=nullptr;
void LogBlocked(const char* phase,uintptr_t attack) {
    // Retaliation may retry every simulation step; sample the rejection instead of flooding the journal.
    static DWORD last=0;DWORD now=GetTickCount();
    if(now-last>=1000) {last=now;wc3::Trace("FPS blocked native attack phase=%s component=%08X",phase,attack);}
}
bool BlockScriptOrder(wc3::Handle unit,int order,wc3::Handle target=0) {
    uintptr_t object=wc3::UnitObject(unit),actor=ActorObject();
    bool controlled=actor && object && (object==actor || (passiveUnit && passiveUnit(object)));
    // Reject attack/attack-ground and smart-unit pursuit before a map's order enters the simulation.
    // Smart item pickup remains native; hero spell IDs do not share these basic order IDs.
    bool attack=order==851983 || order==851984 || (order==851971 && target && wc3::UnitObject(target));
    // Map-issued basic attacks obey the same F9 policy as automatic acquisition.
    auto protectedUnit=eligibilityVerified && protectedActor ? protectedActor() : 0;
    if(attack && target==protectedUnit && protectedUnit && wc3::IsUnitEnemy(protectedUnit,wc3::GetOwningPlayer(unit)))return true;
    if(!controlled || !attack)return false;
    LogBlocked("script",*reinterpret_cast<uintptr_t*>(object+0x1E8));return true;
}
int OrderId(int text) {
    using Convert=int (__fastcall*)(int,uintptr_t);
    return reinterpret_cast<Convert>(gameBase+0x3BAD70)(text,0);
}
BOOL __cdecl TargetIdHook(wc3::Handle unit,int order,wc3::Handle target) {
    return BlockScriptOrder(unit,order,target) ? FALSE : targetId(unit,order,target);
}
BOOL __cdecl TargetStringHook(wc3::Handle unit,int order,wc3::Handle target) {
    return BlockScriptOrder(unit,OrderId(order),target) ? FALSE : targetString(unit,order,target);
}
BOOL __cdecl PointIdHook(wc3::Handle unit,int order,float* x,float* y) {
    return BlockScriptOrder(unit,order) ? FALSE : pointId(unit,order,x,y);
}
BOOL __cdecl PointStringHook(wc3::Handle unit,int order,float* x,float* y) {
    return BlockScriptOrder(unit,OrderId(order)) ? FALSE : pointString(unit,order,x,y);
}
int __fastcall CheckOrderHook(uintptr_t attack,uintptr_t,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d) {
    // Use the native disabled-attack rejection before attack/chase behavior is queued, without changing saved counters.
    if(ControlledAttack(attack)) {LogBlocked("check",attack);return 0xDD;}
    return checkOrder(attack,a,b,c,d);
}
void __fastcall StartOrderHook(uintptr_t attack,uintptr_t,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,
    uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j) {
    // Retaliation can enter directly after validation; refuse only the controlled unit's basic attack component.
    if(ControlledAttack(attack)) {LogBlocked("start",attack);return;}
    startOrder(attack,a,b,c,d,e,f,g,h,i,j);
}
float* __fastcall UnitRangeHook(uintptr_t unit,uintptr_t,float* result) {
    uintptr_t actor=ActorObject();
    if (actor && (unit==actor || (passiveUnit && passiveUnit(unit)))) { *result=0;return result; }return unitRange(unit,result);
}
float* __fastcall AttackRangeHook(uintptr_t attack,uintptr_t,float* result) {
    if (ControlledAttack(attack)) { *result=0;return result; }return attackRange(attack,result);
}
}
void FpsCombatGuard::Install(uintptr_t base,wc3::Handle (*actor)(),bool (*unitPredicate)(uintptr_t),bool (*attackPredicate)(uintptr_t),HookInstaller hook,wc3::Handle (*protectedUnit)()) {
    gameBase=base;viewActor=actor;protectedActor=protectedUnit;
    // Verified 1.26a target eligibility: thiscall(attack,target,int*), ret 8. Other versions retain native behavior.
    const unsigned char eligibilitySignature[]={0x83,0xEC,0x18,0x53,0x55,0x56,0x57,0x8B,0xF9};
    if(!memcmp(reinterpret_cast<void*>(base+0xC93E0),eligibilitySignature,sizeof(eligibilitySignature)))
        eligibilityVerified=hook(reinterpret_cast<void*>(base+0xC93E0),reinterpret_cast<void*>(EligibilityHook),reinterpret_cast<void**>(&nativeEligibility));
    wc3::Log("Unit-view enemy targeting guard installed=%d",eligibilityVerified);
    // The FPS squad supplies its passive policy; normal units continue through the native AI getters.
    passiveUnit=unitPredicate;passiveAttack=attackPredicate;
    const unsigned char targetSignature[]={0x8B,0x41,0x70,0x23,0x41,0x6C};
    if (memcmp(reinterpret_cast<void*>(base+0x7C2C0),targetSignature,sizeof(targetSignature))) {
        wc3::Log("FPS native attack signature mismatch; guard not installed");return;
    }
    targetVerified=hook(reinterpret_cast<void*>(base+0x7C2C0),reinterpret_cast<void*>(AttackTargetHook),reinterpret_cast<void**>(&nativeTarget));
    wc3::Log("FPS native attack target lookup installed=%d",targetVerified);
    hook(reinterpret_cast<void*>(base+0x276A00),reinterpret_cast<void*>(UnitRangeHook),reinterpret_cast<void**>(&unitRange));
    hook(reinterpret_cast<void*>(base+0xC6060),reinterpret_cast<void*>(AttackRangeHook),reinterpret_cast<void**>(&attackRange));
    // Both entry points test the attack-disable counter natively; validate their separate x86 argument layouts.
    const unsigned char checkSignature[]={0x56,0x8B,0xF1,0x8B,0x06,0x8B,0x90,0xA0,0x00,0x00,0x00};
    const unsigned char startSignature[]={0x56,0x8B,0xF1,0x83,0x7E,0x40,0x00};
    if(memcmp(reinterpret_cast<void*>(base+0xC5E30),checkSignature,sizeof(checkSignature)) ||
        memcmp(reinterpret_cast<void*>(base+0xC6210),startSignature,sizeof(startSignature))) {
        wc3::Log("FPS native attack dispatch signature mismatch; chase guard not installed");return;
    }
    bool checked=hook(reinterpret_cast<void*>(base+0xC5E30),reinterpret_cast<void*>(CheckOrderHook),reinterpret_cast<void**>(&checkOrder));
    bool started=hook(reinterpret_cast<void*>(base+0xC6210),reinterpret_cast<void*>(StartOrderHook),reinterpret_cast<void**>(&startOrder));
    wc3::Log("FPS native attack dispatch guard check=%d start=%d",checked,started);
    // String and numeric JASS orders have distinct entry points; guard both before a single chase step can run.
    const unsigned char numericSignature[]={0x8B,0x4C,0x24,0x04},textSignature[]={0x8B,0x4C,0x24,0x08,0xE8};
    const unsigned char convertSignature[]={0x56,0x8B,0x35};
    if(memcmp(reinterpret_cast<void*>(base+0x3C89D0),numericSignature,sizeof(numericSignature)) ||
        memcmp(reinterpret_cast<void*>(base+0x3C8940),numericSignature,sizeof(numericSignature)) ||
        memcmp(reinterpret_cast<void*>(base+0x3C8E20),textSignature,sizeof(textSignature)) ||
        memcmp(reinterpret_cast<void*>(base+0x3C8DA0),textSignature,sizeof(textSignature)) ||
        memcmp(reinterpret_cast<void*>(base+0x3BAD70),convertSignature,sizeof(convertSignature))) {
        wc3::Log("FPS map attack order signature mismatch; script chase guard not installed");return;
    }
    bool targetNumeric=hook(reinterpret_cast<void*>(wc3::IssueTargetOrderById),reinterpret_cast<void*>(TargetIdHook),reinterpret_cast<void**>(&targetId));
    bool pointNumeric=hook(reinterpret_cast<void*>(wc3::IssuePointOrderById),reinterpret_cast<void*>(PointIdHook),reinterpret_cast<void**>(&pointId));
    bool targetText=hook(reinterpret_cast<void*>(base+0x3C8E20),reinterpret_cast<void*>(TargetStringHook),reinterpret_cast<void**>(&targetString));
    bool pointText=hook(reinterpret_cast<void*>(base+0x3C8DA0),reinterpret_cast<void*>(PointStringHook),reinterpret_cast<void**>(&pointString));
    wc3::Log("FPS map attack order guard targetId=%d pointId=%d targetString=%d pointString=%d",targetNumeric,pointNumeric,targetText,pointText);
}
bool FpsCombatGuard::BlocksIncomingAttack(uintptr_t attack,uintptr_t target) {
    auto actor=eligibilityVerified && protectedActor ? protectedActor() : 0;
    if(!actor || !attack || !target || target!=wc3::UnitObject(actor))return false;
    // Owner resolution mirrors GetOwningPlayer: the attack's owner unit, then its native player-index virtual.
    auto source=*reinterpret_cast<uintptr_t*>(attack+0x30);
    if(!source)source=reinterpret_cast<uintptr_t (__thiscall*)(uintptr_t)>(gameBase+0x472890)(attack);
    if(!source)return false;
    auto table=*reinterpret_cast<uintptr_t*>(source);
    int player=reinterpret_cast<int (__thiscall*)(uintptr_t)>(*reinterpret_cast<uintptr_t*>(table+0xEC))(source);
    return player>=0 && player<16 && wc3::IsUnitEnemy(actor,wc3::Player(player));
}
bool FpsCombatGuard::HasAttackTarget(wc3::Handle unit) {
    // Never call the version-specific target getter if its native signature failed validation.
    if (!targetVerified) return false;
    uintptr_t object=wc3::UnitObject(unit),attack=object ? *reinterpret_cast<uintptr_t*>(object+0x1E8) : 0;
    // Cleanup observes the native target without changing the reference Warcraft owns.
    uintptr_t target=attack ? nativeTarget(attack) : 0;
    return target!=0;
}
void FpsCombatGuard::CancelAttacks(wc3::Handle unit) {
    // Move orders suppress retaliation natively and must survive passive squad travel.
    if (wc3::GetUnitCurrentOrder(unit)!=851986) CancelOrders(unit);
}
void FpsCombatGuard::CancelOrders(wc3::Handle unit) {
    if (!targetVerified) return;
    int order=wc3::GetUnitCurrentOrder(unit);
    uintptr_t object=wc3::UnitObject(unit),attack=object ? *reinterpret_cast<uintptr_t*>(object+0x1E8) : 0;
    uintptr_t target=attack ? nativeTarget(attack) : 0;
    static NativeOrderCancellation cancellation;
    // Retaliation/auto-acquisition has a native target even when GetUnitCurrentOrder reports zero.
    // Clear that behavior before its windup completes; do not flood idle units with stop events.
    // A retained target after Stop is not a new attack. Repeated Stop cancels the player's TPS animation in Dota.
    if (cancellation.Observe(unit,target,order)) {
        wc3::IssueImmediateOrderById(unit,851972);
        // The native target reference can linger after stop; keep diagnostics bounded during close combat.
        static DWORD lastLog=0;DWORD now=GetTickCount();
        if (now-lastLog>=1000) { lastLog=now;wc3::Log("FPS canceled native order=%d target=%08X unit=%08X",order,target,unit); }
    }
}
