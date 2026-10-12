#include "NativeWeaponAttack.hpp"
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
uintptr_t ControlledObject() {return wc3::UnitObject(controlledActor ? controlledActor() : 0);}
void __fastcall GuardImpact(uintptr_t attack,uintptr_t,uintptr_t target,int a,int b,int c,int d,int e) {
    auto actor=ControlledObject();
    // A retained native AI target can still reach impact without a new order. Only our scoped manual contact may fire.
    bool controlled=(actor && *reinterpret_cast<uintptr_t*>(actor+0x1E8)==attack) || (passiveAttack && passiveAttack(attack));
    if(controlled && !(contact && contact->attack==attack)) {
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
bool Execute(wc3::Handle attacker,wc3::Handle victim,uintptr_t object,uintptr_t attack,uintptr_t target,float damage,float distance,int kind,bool projectile) {
    Contact current={attack,damage,distance,false,false};Contact* previous=contact;
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
        if(wc3::UnitObject(attacker)==object && wc3::UnitObject(victim)==target &&
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
            reinterpret_cast<Finish>(gameBase+0x276330)(object,wc3::UnitObject(victim)==target ? target : 0);
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
    std::memcpy(&relative,reinterpret_cast<void*>(base+0xCF82B),4);
    bool verified=!std::memcmp(reinterpret_cast<void*>(base+0xCF660),impact,sizeof(impact)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0xC6940),roll,sizeof(roll)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x281580),prepare,sizeof(prepare)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x276330),finish,sizeof(finish)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0xC6B30),range,sizeof(range)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0xD0225),cleanup,sizeof(cleanup)) && 0xCF82F+relative==0xC6940;
    available=verified && hook(reinterpret_cast<void*>(base+0xC6940),reinterpret_cast<void*>(DamageRoll),reinterpret_cast<void**>(&originalRoll)) &&
        hook(reinterpret_cast<void*>(base+0xC6B30),reinterpret_cast<void*>(ContactRange),reinterpret_cast<void**>(&originalRange)) &&
        hook(reinterpret_cast<void*>(base+0xCF660),reinterpret_cast<void*>(GuardImpact),reinterpret_cast<void**>(&originalImpact)) &&
        hook(reinterpret_cast<void*>(base+0x281580),reinterpret_cast<void*>(GuardPrepare),reinterpret_cast<void**>(&originalPrepare));
    wc3::Log("Native weapon attack effects verified=%d installed=%d",verified,available);
}
namespace {
bool Dispatch(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance,bool projectile) {
    if(!available || contact || !std::isfinite(damage) || damage<=0 || !std::isfinite(contactDistance) || contactDistance<0)return false;
    uintptr_t object=wc3::UnitObject(attacker),victim=wc3::UnitObject(target),attack=0;int kind=0;
    if(!object || !victim || !Read(object+0x1E8,attack) || !Read(attack+0xDC,kind) || kind<1 || kind>8)return false;
    // Once the pipeline has run, even an evaded/immune hit is handled: never add a fallback second hit.
    bool substituted=Execute(attacker,target,object,attack,victim,damage,contactDistance,kind,projectile);
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
