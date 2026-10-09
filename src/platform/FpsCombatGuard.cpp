#include "FpsCombatGuard.hpp"
#include <cstring>

namespace {
uintptr_t gameBase=0;
bool targetVerified=false;
wc3::Handle (*viewActor)()=nullptr;
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
float* __fastcall UnitRangeHook(uintptr_t unit,uintptr_t,float* result) {
    uintptr_t actor=ActorObject();
    if (actor && (unit==actor || (passiveUnit && passiveUnit(unit)))) { *result=0;return result; }return unitRange(unit,result);
}
float* __fastcall AttackRangeHook(uintptr_t attack,uintptr_t,float* result) {
    if (ControlledAttack(attack)) { *result=0;return result; }return attackRange(attack,result);
}
}
void FpsCombatGuard::Install(uintptr_t base,wc3::Handle (*actor)(),bool (*unitPredicate)(uintptr_t),bool (*attackPredicate)(uintptr_t),HookInstaller hook) {
    gameBase=base;viewActor=actor;
    // The FPS squad supplies its passive policy; normal units continue through the native AI getters.
    passiveUnit=unitPredicate;passiveAttack=attackPredicate;
    const unsigned char targetSignature[]={0x8B,0x41,0x70,0x23,0x41,0x6C};
    if (memcmp(reinterpret_cast<void*>(base+0x7C2C0),targetSignature,sizeof(targetSignature))) {
        wc3::Log("FPS native attack signature mismatch; guard not installed");return;
    }
    targetVerified=true;
    hook(reinterpret_cast<void*>(base+0x276A00),reinterpret_cast<void*>(UnitRangeHook),reinterpret_cast<void**>(&unitRange));
    hook(reinterpret_cast<void*>(base+0xC6060),reinterpret_cast<void*>(AttackRangeHook),reinterpret_cast<void**>(&attackRange));
}
bool FpsCombatGuard::HasAttackTarget(wc3::Handle unit) {
    // Never call the version-specific target getter if its native signature failed validation.
    if (!targetVerified) return false;
    uintptr_t object=wc3::UnitObject(unit),attack=object ? *reinterpret_cast<uintptr_t*>(object+0x1E8) : 0;
    using Target=uintptr_t (__thiscall*)(uintptr_t);
    uintptr_t target=attack ? reinterpret_cast<Target>(gameBase+0x7C2C0)(attack) : 0;
    return target!=0;
}
void FpsCombatGuard::CancelAttacks(wc3::Handle unit) {
    // Move orders suppress retaliation natively and must survive passive squad travel.
    if (wc3::GetUnitCurrentOrder(unit)!=851986) CancelOrders(unit);
}
void FpsCombatGuard::CancelOrders(wc3::Handle unit) {
    if (!targetVerified) return;
    int order=wc3::GetUnitCurrentOrder(unit);bool target=HasAttackTarget(unit);
    // Retaliation/auto-acquisition has a native target even when GetUnitCurrentOrder reports zero.
    // Clear that behavior before its windup completes; do not flood idle units with stop events.
    if (target || (order && order!=851972)) {
        wc3::IssueImmediateOrderById(unit,851972);
        // The native target reference can linger after stop; keep diagnostics bounded during close combat.
        static DWORD lastLog=0;DWORD now=GetTickCount();
        if (now-lastLog>=1000) { lastLog=now;wc3::Log("FPS canceled native order=%d target=%d unit=%08X",order,target,unit); }
    }
}
