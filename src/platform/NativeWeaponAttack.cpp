#include "NativeWeaponAttack.hpp"
#include "FpsCombatGuard.hpp"
#include "NativeAttackTarget.hpp"
#include <cmath>
#include <cstring>

namespace {
uintptr_t gameBase=0;
bool available=false;
struct Contact {uintptr_t attack;float damage,distance;bool substituted,inImpact;};
thread_local Contact* contact=nullptr;
using Roll=float* (__thiscall*)(uintptr_t,float*);
Roll originalRoll=nullptr;
using Range=float* (__thiscall*)(uintptr_t,float*,int);
Range originalRange=nullptr;
wc3::Handle (*controlledActor)()=nullptr;
bool (*passiveUnit)(uintptr_t)=nullptr,(*passiveAttack)(uintptr_t)=nullptr;
using Impact=void (__thiscall*)(uintptr_t,uintptr_t,int,int,int,int,int);
using Prepare=void (__thiscall*)(uintptr_t,uintptr_t,float*,float*);
Impact originalImpact=nullptr;
Prepare originalPrepare=nullptr;
using Windup=void (__thiscall*)(uintptr_t,int);
using AttackPose=void (__thiscall*)(uintptr_t,uintptr_t);
Windup originalWindup=nullptr;
AttackPose originalAttackPose=nullptr;
uintptr_t ControlledObject() {return wc3::UnitObject(controlledActor ? controlledActor() : 0);}
bool SuppressAutonomous(uintptr_t attack) {
    auto actor=ControlledObject();
    bool controlled=(actor && *reinterpret_cast<uintptr_t*>(actor+0x1E8)==attack) || (passiveAttack && passiveAttack(attack));
    return controlled && !(contact && contact->attack==attack);
}
void __fastcall GuardWindup(uintptr_t attack,uintptr_t,int phase) {
    // An existing target bypasses order validation and restarts its own attack timer. Stop that cycle before it winds up.
    if(!SuppressAutonomous(attack))originalWindup(attack,phase);
}
void __fastcall GuardAttackPose(uintptr_t attack,uintptr_t,uintptr_t actor) {
    // Blocking damage alone leaves native attack animations/rate changes visible on ranged heroes.
    // Explicit TPS animations use SetUnitAnimation instead, so this native AI pose can be suppressed independently.
    if(!SuppressAutonomous(attack))originalAttackPose(attack,actor);
    else {
        static DWORD last=0;DWORD now=GetTickCount();
        if(now-last>=1000){last=now;wc3::Trace("FPS blocked autonomous attack pose component=%08X",attack);}
    }
}
void __fastcall GuardImpact(uintptr_t attack,uintptr_t,uintptr_t target,int a,int b,int c,int d,int e) {
    // A retained AI target can reach impact without a new order; F9 protection also covers a prior hostile wind-up.
    if(SuppressAutonomous(attack) || FpsCombatGuard::BlocksIncomingAttack(attack,target)) {
        static DWORD last=0;DWORD now=GetTickCount();
        if(now-last>=1000){last=now;wc3::Trace("FPS blocked autonomous impact component=%08X",attack);}return;
    }
    originalImpact(attack,target,a,b,c,d,e);
}
void __fastcall GuardPrepare(uintptr_t actor,uintptr_t,uintptr_t target,float* a,float* b) {
    // Suppress autonomous wind-up callbacks while preserving the explicitly dispatched manual attack/passives.
    // Follow-only squad members keep the same passive policy as the basic-order guard.
    bool controlled=actor==ControlledObject() || (passiveUnit && passiveUnit(actor));
    if(controlled && !(contact && contact->attack==*reinterpret_cast<uintptr_t*>(actor+0x1E8)))return;
    // An already-started hostile contact must respect a newly enabled F9 policy before native attack callbacks.
    if(FpsCombatGuard::BlocksIncomingAttack(*reinterpret_cast<uintptr_t*>(actor+0x1E8),target))return;
    originalPrepare(actor,target,a,b);
}
float* __fastcall ContactRange(uintptr_t attack,uintptr_t,float* result,int index) {
    auto value=originalRange(attack,result,index);
    // The FPS ray already verified range/terrain. Extend only its synchronous impact check,
    // preserving the hero's ordinary RTS range and native melee-only passive eligibility.
    if(contact && contact->inImpact && contact->attack==attack && index==0 && *value<contact->distance)
        *value=contact->distance;
    return value;
}
float* __fastcall DamageRoll(uintptr_t attack,uintptr_t,float* result) {
    auto value=originalRoll(attack,result);
    // Replace only this contact's base roll; native crits, orbs and damage triggers still process it afterwards.
    if(contact && contact->inImpact && contact->attack==attack && !contact->substituted) {
        *value=contact->damage;contact->substituted=true;
    }
    return value;
}
template<class T> bool Read(uintptr_t address,T& value) {
    SIZE_T got=0;return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&value,sizeof(value),&got) && got==sizeof(value);
}
bool Execute(wc3::Handle attacker,const NativeAttackTarget& victim,uintptr_t object,uintptr_t attack,float damage,float distance,int kind,bool projectile) {
    Contact current={attack,damage,distance,false,false};Contact* previous=contact;
    uintptr_t target=victim.object;
    using Finish=void (__thiscall*)(uintptr_t,uintptr_t);
    // Retain the authored ranged kind for TPS so Warcraft creates its own missile from current map data.
    float zero=0;int instantKind=projectile ? kind : kind==1 ? 1 : 4;
    contact=&current;
    __try {
        // 1 is melee and 4 is instant ranged. FPS changes only this impact; TPS retains its authored missile kind.
        // Neither path assigns an AI order; preparation and missile allocation remain on the game thread.
        *reinterpret_cast<int*>(attack+0xDC)=instantKind;
        // Native wind-up rolls Bash/critical chances before impact. Its callbacks also notify map attack triggers.
        reinterpret_cast<Prepare>(gameBase+0x281580)(object,target,&zero,&zero);
        uintptr_t currentAttack=0;
        // Native impact accepts a CWidget, including gates. Revalidate its original typed identity after callbacks.
        if(wc3::UnitObject(attacker)==object && victim.Valid() &&
            Read(object+0x1E8,currentAttack) && currentAttack==attack) {
            current.inImpact=true;
            reinterpret_cast<Impact>(gameBase+0xCF660)(attack,target,0,0,1,0,1);
        }
    } __finally {
        contact=previous;
        uintptr_t stillOwned=0;
        // Attack callbacks can transform/remove the hero; never restore a freed/replaced attack component.
        if(wc3::UnitObject(attacker)==object) {
            // Release native per-attack flags/counters, just as the ordinary attack animation ends.
            reinterpret_cast<Finish>(gameBase+0x276330)(object,victim.Valid() ? target : 0);
            int currentKind=0;
            if(wc3::UnitObject(attacker)==object && Read(object+0x1E8,stillOwned) && stillOwned==attack &&
                Read(attack+0xDC,currentKind) && currentKind==instantKind)
                *reinterpret_cast<int*>(attack+0xDC)=kind;
        }
    }
    return current.substituted;
}
}
void NativeWeaponAttack::Install(uintptr_t base,HookInstaller hook,wc3::Handle (*actor)(),bool (*unitPolicy)(uintptr_t),bool (*attackPolicy)(uintptr_t)) {
    gameBase=base;
    controlledActor=actor;
    passiveUnit=unitPolicy;passiveAttack=attackPolicy;
    const unsigned char impact[]={0x6A,0xFF,0x68};
    const unsigned char roll[]={0x51,0x56,0x57,0x68};
    const unsigned char cleanup[]={0xC2,0x18,0x00};int relative=0;
    const unsigned char prepare[]={0x51,0x57,0x8B,0xF9};
    const unsigned char finish[]={0x8B,0x81,0xE0,0x01,0x00,0x00};
    const unsigned char range[]={0x56,0x8B,0xF1,0x8B,0x86,0xC0,0x02,0x00,0x00};
    const unsigned char windup[]={0x53,0x55,0x56,0x8B,0xF1,0x8B,0x46,0x70};
    const unsigned char pose[]={0x83,0xEC,0x08,0x56,0x57,0x8B,0xF9};
    std::memcpy(&relative,reinterpret_cast<void*>(base+0xCF82B),4);
    bool verified=!std::memcmp(reinterpret_cast<void*>(base+0xCF660),impact,sizeof(impact)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0xC6940),roll,sizeof(roll)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x281580),prepare,sizeof(prepare)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x276330),finish,sizeof(finish)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0xC6B30),range,sizeof(range)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0xCF5C0),windup,sizeof(windup)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0xC8930),pose,sizeof(pose)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0xD0225),cleanup,sizeof(cleanup)) && 0xCF82F+relative==0xC6940;
    available=verified && hook(reinterpret_cast<void*>(base+0xC6940),reinterpret_cast<void*>(DamageRoll),reinterpret_cast<void**>(&originalRoll)) &&
        hook(reinterpret_cast<void*>(base+0xC6B30),reinterpret_cast<void*>(ContactRange),reinterpret_cast<void**>(&originalRange)) &&
        hook(reinterpret_cast<void*>(base+0xCF660),reinterpret_cast<void*>(GuardImpact),reinterpret_cast<void**>(&originalImpact)) &&
        hook(reinterpret_cast<void*>(base+0x281580),reinterpret_cast<void*>(GuardPrepare),reinterpret_cast<void**>(&originalPrepare)) &&
        hook(reinterpret_cast<void*>(base+0xCF5C0),reinterpret_cast<void*>(GuardWindup),reinterpret_cast<void**>(&originalWindup)) &&
        hook(reinterpret_cast<void*>(base+0xC8930),reinterpret_cast<void*>(GuardAttackPose),reinterpret_cast<void**>(&originalAttackPose));
    wc3::Log("Native weapon attack effects verified=%d installed=%d",verified,available);
}
namespace {
bool Dispatch(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance,bool projectile) {
    if(!available || contact || !std::isfinite(damage) || damage<=0 || !std::isfinite(contactDistance) || contactDistance<0)return false;
    // Resolve the widget type once; a destructable handle must never be validated as a CUnit.
    auto victim=NativeAttackTarget::Resolve(target);
    uintptr_t object=wc3::UnitObject(attacker),attack=0;int kind=0;
    if(!object || !victim.Valid() || !Read(object+0x1E8,attack) || !Read(attack+0xDC,kind) || kind<1 || kind>8)return false;
    // Once the pipeline has run, even an evaded/immune hit is handled: never add a fallback second hit.
    bool substituted=Execute(attacker,victim,object,attack,damage,contactDistance,kind,projectile);
    wc3::Log("Weapon attack effects attacker=%08X target=%08X base=%.1f substituted=%d nativeKind=%d distance=%.1f projectile=%d",attacker,target,damage,substituted,kind,contactDistance,projectile);
    return true;
}
}
bool NativeWeaponAttack::Apply(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance) {
    return Dispatch(attacker,target,damage,contactDistance,false);
}
bool NativeWeaponAttack::Launch(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance) {
    return Dispatch(attacker,target,damage,contactDistance,true);
}
int NativeWeaponAttack::PrimaryKind(wc3::Handle attacker) {
    uintptr_t object=wc3::UnitObject(attacker),attack=0;int kind=0;
    return object && Read(object+0x1E8,attack) && Read(attack+0xDC,kind) && kind>=1 && kind<=8 ? kind : 0;
}
float NativeWeaponAttack::PrimaryRange(wc3::Handle attacker) {
    uintptr_t object=wc3::UnitObject(attacker),attack=0;float range=0;
    // Read the original statistics getter, bypassing the acquisition guard's deliberate zero range.
    if(available && object && Read(object+0x1E8,attack) && attack)originalRange(attack,&range,0);
    return std::isfinite(range) && range>0 && range<=10000 ? range : 0;
}
int NativeWeaponAttack::AttackType(wc3::Handle attacker) {
    uintptr_t object=wc3::UnitObject(attacker),attack=0;int type=0;
    // Disabling passives must not switch the armor multiplier from the creature's attack type to spell damage.
    return object && Read(object+0x1E8,attack) && Read(attack+0xF4,type) && type>=0 && type<=6 ? type : 0;
}
