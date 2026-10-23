#include "GameBuild.hpp"
#include "NativeWeaponAttack.hpp"
#include "FpsCombatGuard.hpp"
#include "NativeAttackTarget.hpp"
#include <cmath>
#include <cstring>
#include <intrin.h>

namespace {
uintptr_t gameBase=0;
bool available=false;
struct Contact {uintptr_t attack;float damage,distance;bool substituted,inImpact,silent;};
thread_local Contact* contact=nullptr;
thread_local uintptr_t impactAttack=0;
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
using InstantEffect=uintptr_t (__fastcall*)(uintptr_t,uintptr_t,const uint32_t*,int,int);
InstantEffect originalInstantEffect=nullptr;
uintptr_t __fastcall GuardInstantEffect(uintptr_t target,uintptr_t kind,const uint32_t* attachments,int count,int flags) {
    auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    // Instant ranged damage also spawns the unit's ordinary attack art, whose MDX events can play arrow audio.
    // Match its two verified call sites, not generic spell/passive art or another unit's nested impact.
    if(contact && contact->inImpact && contact->silent && impactAttack==contact->attack && kind==0x10 &&
        (caller==gameBase+wc3::Rva(0xCF8FD) || caller==gameBase+wc3::Rva(0xCF988))) {
        static DWORD previous=0;DWORD now=GetTickCount();
        if(!previous || now-previous>=1000){previous=now;wc3::Trace("FPS suppressed native instant attack art attack=%08X caller=%08X",impactAttack,caller-gameBase);}
        return 0;
    }
    return originalInstantEffect(target,kind,attachments,count,flags);
}
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
    auto previous=impactAttack;impactAttack=attack;
    __try {originalImpact(attack,target,a,b,c,d,e);}
    __finally {impactAttack=previous;} // Reentrant map attacks keep their own presentation and audio.
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
bool Execute(wc3::Handle attacker,const NativeAttackTarget& victim,uintptr_t object,uintptr_t attack,float damage,float distance,int kind,bool projectile,bool nativeSound) {
    Contact current={attack,damage,distance,false,false,!nativeSound && !projectile};Contact* previous=contact;
    uintptr_t target=victim.object;
    using Finish=void (__thiscall*)(uintptr_t,uintptr_t);
    // Retain the authored ranged kind for TPS so Warcraft creates its own missile from current map data.
    float zero=0;int instantKind=projectile ? kind : kind==1 ? 1 : 4;
    const int weaponSound=*reinterpret_cast<int*>(attack+0xE8);
    contact=&current;
    __try {
        // 1 is melee and 4 is instant ranged. FPS changes only this impact; TPS retains its authored missile kind.
        // Neither path assigns an AI order; preparation and missile allocation remain on the game thread.
        *reinterpret_cast<int*>(attack+0xDC)=instantKind;
        // Verified 1.26a primary weapon sound enum: NONE silences this contact without bypassing passives.
        if(!nativeSound)*reinterpret_cast<int*>(attack+0xE8)=0;
        // Native wind-up rolls Bash/critical chances before impact. Its callbacks also notify map attack triggers.
        reinterpret_cast<Prepare>(gameBase + wc3::Rva(0x281580))(object,target,&zero,&zero);
        uintptr_t currentAttack=0;
        // Native impact accepts a CWidget, including gates. Revalidate its original typed identity after callbacks.
        if(wc3::UnitObject(attacker)==object && victim.Valid() &&
            Read(object+0x1E8,currentAttack) && currentAttack==attack) {
            current.inImpact=true;
            reinterpret_cast<Impact>(gameBase + wc3::Rva(0xCF660))(attack,target,0,0,1,0,1);
        }
    } __finally {
        contact=previous;
        uintptr_t stillOwned=0;
        // Attack callbacks can transform/remove the hero; never restore a freed/replaced attack component.
        if(wc3::UnitObject(attacker)==object) {
            // Release native per-attack flags/counters, just as the ordinary attack animation ends.
            reinterpret_cast<Finish>(gameBase + wc3::Rva(0x276330))(object,victim.Valid() ? target : 0);
            int currentKind=0;
            if(wc3::UnitObject(attacker)==object && Read(object+0x1E8,stillOwned) && stillOwned==attack &&
                Read(attack+0xDC,currentKind)) {
                if(currentKind==instantKind)*reinterpret_cast<int*>(attack+0xDC)=kind;
                // Preserve changes made by map callbacks, and never write a replaced/freed component.
                if(!nativeSound && *reinterpret_cast<int*>(attack+0xE8)==0)*reinterpret_cast<int*>(attack+0xE8)=weaponSound;
            }
        }
    }
    return current.substituted;
}
}
void NativeWeaponAttack::Install(uintptr_t base,HookInstaller hook,wc3::Handle (*actor)(),bool (*unitPolicy)(uintptr_t),bool (*attackPolicy)(uintptr_t)) {
    // This adapter requires engine internals that are not yet verified on 1.27a.
    if(wc3::ActiveBuild()==wc3::GameBuildKind::Classic127a) return;

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
    const unsigned char sound[]={0x8B,0x84,0xBE,0xE8,0,0,0}; // Impact reads the primary weapon sound at +0xE8.
    const unsigned char effect[]={0x53,0x8B,0x5C,0x24,0x0C};
    int firstEffect=0,reflectedEffect=0;
    std::memcpy(&firstEffect,reinterpret_cast<void*>(base+wc3::Rva(0xCF8F9)),4);
    std::memcpy(&reflectedEffect,reinterpret_cast<void*>(base+wc3::Rva(0xCF984)),4);
    std::memcpy(&relative,reinterpret_cast<void*>(base + wc3::Rva(0xCF82B)),4);
    bool verified=!std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0xCF660)),impact,sizeof(impact)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0xC6940)),roll,sizeof(roll)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x281580)),prepare,sizeof(prepare)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x276330)),finish,sizeof(finish)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0xC6B30)),range,sizeof(range)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0xCF5C0)),windup,sizeof(windup)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0xC8930)),pose,sizeof(pose)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0xCF817)),sound,sizeof(sound)) &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x2AAD70)),effect,sizeof(effect)) &&
        *reinterpret_cast<unsigned char*>(base+wc3::Rva(0xCF8F8))==0xE8 && 0xCF8FD+firstEffect==0x2AAD70 &&
        *reinterpret_cast<unsigned char*>(base+wc3::Rva(0xCF983))==0xE8 && 0xCF988+reflectedEffect==0x2AAD70 &&
        !std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0xD0225)),cleanup,sizeof(cleanup)) && 0xCF82F+relative==0xC6940;
    available=verified && hook(reinterpret_cast<void*>(base + wc3::Rva(0xC6940)),reinterpret_cast<void*>(DamageRoll),reinterpret_cast<void**>(&originalRoll)) &&
        hook(reinterpret_cast<void*>(base + wc3::Rva(0xC6B30)),reinterpret_cast<void*>(ContactRange),reinterpret_cast<void**>(&originalRange)) &&
        hook(reinterpret_cast<void*>(base + wc3::Rva(0xCF660)),reinterpret_cast<void*>(GuardImpact),reinterpret_cast<void**>(&originalImpact)) &&
        hook(reinterpret_cast<void*>(base + wc3::Rva(0x281580)),reinterpret_cast<void*>(GuardPrepare),reinterpret_cast<void**>(&originalPrepare)) &&
        hook(reinterpret_cast<void*>(base + wc3::Rva(0xCF5C0)),reinterpret_cast<void*>(GuardWindup),reinterpret_cast<void**>(&originalWindup)) &&
        hook(reinterpret_cast<void*>(base + wc3::Rva(0xC8930)),reinterpret_cast<void*>(GuardAttackPose),reinterpret_cast<void**>(&originalAttackPose)) &&
        hook(reinterpret_cast<void*>(base + wc3::Rva(0x2AAD70)),reinterpret_cast<void*>(GuardInstantEffect),reinterpret_cast<void**>(&originalInstantEffect));
    wc3::Log("Native weapon attack effects verified=%d installed=%d",verified,available);
    wc3::Log("Native instant attack art/audio filter installed=%d",available);
}
namespace {
bool Dispatch(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance,bool projectile,bool nativeSound) {
    if(!available || contact || !std::isfinite(damage) || damage<=0 || !std::isfinite(contactDistance) || contactDistance<0)return false;
    // Resolve the widget type once; a destructable handle must never be validated as a CUnit.
    auto victim=NativeAttackTarget::Resolve(target);
    uintptr_t object=wc3::UnitObject(attacker),attack=0;int kind=0;
    if(!object || !victim.Valid() || !Read(object+0x1E8,attack) || !Read(attack+0xDC,kind) || kind<1 || kind>8)return false;
    // Once the pipeline has run, even an evaded/immune hit is handled: never add a fallback second hit.
    bool substituted=Execute(attacker,victim,object,attack,damage,contactDistance,kind,projectile,nativeSound);
    wc3::Log("Weapon attack effects attacker=%08X target=%08X base=%.1f substituted=%d nativeKind=%d distance=%.1f projectile=%d nativeSound=%d",attacker,target,damage,substituted,kind,contactDistance,projectile,nativeSound);
    return true;
}
}
bool NativeWeaponAttack::Apply(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance,bool nativeSound) {
    return Dispatch(attacker,target,damage,contactDistance,false,nativeSound);
}
bool NativeWeaponAttack::Launch(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance) {
    return Dispatch(attacker,target,damage,contactDistance,true,true);
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
