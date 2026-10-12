#include "HeroMeleeAttack.hpp"
#include "NativeWeaponAttack.hpp"
#include "WeaponTargetFilter.hpp"
#include "../combat/CombatDamage.hpp"
#include <cmath>

bool HeroMeleeAttack::Supported(wc3::Handle unit) {
    // The first prototype supports native primary melee bodies, including campaign Arthas.
    uintptr_t object=wc3::UnitObject(unit),attack=object ? *reinterpret_cast<uintptr_t*>(object+0x1E8) : 0;
    return attack && *reinterpret_cast<int*>(attack+0xDC)==1;
}
bool HeroMeleeAttack::Begin(wc3::Handle unit,double now,const GameplaySettings& settings,const UnitStatus& status) {
    if(status.disarmed || status.incapacitated || !Supported(unit) ||
        !timeline_.Begin(now,settings.thirdPerson.contactDelay,settings.thirdPerson.attackInterval,status.attackSpeed))return false;
    wc3::Animate(unit,"attack");pose_=Pose::Attack;
    wc3::Log("Third-person swing unit=%08X attack=%.1f speed=%.2f",unit,status.attackDamage,status.attackSpeed);return true;
}
bool HeroMeleeAttack::Tick(wc3::Handle unit,double now,float yaw,float feet,bool moving,bool casting,
    const GameplaySettings& settings,const UnitStatus& status,UnitHitboxes& units,DestructableHitboxes& destructables) {
    if(casting || status.incapacitated || status.disarmed || status.hidden || status.contained) {
        Cancel();return false;
    }
    bool hit=timeline_.Contact(now) && Contact(unit,yaw,feet,settings,units,destructables);
    if(!timeline_.Busy(now)) {
        auto next=moving ? Pose::Walk : Pose::Stand;
        // Only animation transitions write the sprite; learned spells retain native animation ownership.
        if(next!=pose_) {wc3::Animate(unit,moving ? "walk" : "stand");pose_=next;}
    }
    return hit;
}
bool HeroMeleeAttack::Contact(wc3::Handle actor,float yaw,float feet,const GameplaySettings& settings,
    UnitHitboxes& units,DestructableHitboxes& destructables) {
    const auto& tuning=settings.thirdPerson;
    float origin[]={wc3::Real(wc3::GetUnitX(actor)),wc3::Real(wc3::GetUnitY(actor)),feet+tuning.attackHeight};
    float x=origin[0],y=origin[1],radius=tuning.attackRange+512,nearest=tuning.attackRange;
    auto group=wc3::CreateGroup();wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    std::vector<wc3::Handle> candidates;
    while(auto unit=wc3::FirstOfGroup(group)) {wc3::GroupRemoveUnit(group,unit);candidates.push_back(unit);}
    wc3::DestroyGroup(group);
    auto objects=wc3::NearbyDestructables(x,y,radius);
    wc3::Handle target=0;bool destructable=false;
    // Sweep the authored forward arc. Each sample competes with terrain/gates, so a side ray cannot pierce a wall.
    for(int ray=0;ray<=10;++ray) {
        float angle=(yaw-tuning.attackArc*.5f+tuning.attackArc*ray/10)*.01745329252f;
        float direction[]={std::cos(angle),std::sin(angle),0},limit=tuning.attackRange;
        for(float t=8;t<=limit;t+=8)if(wc3::Ground(x+direction[0]*t,y+direction[1]*t)>=origin[2]) {limit=t;break;}
        wc3::Handle contact=0;bool isObject=false;
        for(auto unit:candidates) {
            if(unit==actor || wc3::IsUnitHidden(unit) || wc3::IsUnitLoaded(unit) ||
                wc3::Real(wc3::GetUnitState(unit,0))<=.405f || !wc3::IsUnitVisible(unit,wc3::GetLocalPlayer()))continue;
            if(settings.ignorePassableInvulnerableObjects && WeaponTargetFilter::PassThrough(unit,
                reinterpret_cast<uintptr_t>(GetModuleHandleA("Game.dll"))))continue;
            float entry=0;if(units.Intersect(unit,origin,direction,limit,entry)) {limit=entry;contact=unit;isObject=false;}
        }
        for(auto object:objects) {
            float entry=0;if(wc3::Real(wc3::GetDestructableLife(object))>.405f &&
                destructables.Intersect(object,origin,direction,limit,entry)) {limit=entry;contact=object;isObject=true;}
        }
        if(contact && limit<=nearest) {nearest=limit;target=contact;destructable=isObject;}
    }
    if(!target) {wc3::Log("Third-person melee miss unit=%08X yaw=%.1f",actor,yaw);return false;}
    auto life=[&] {return wc3::Real(destructable ? wc3::GetDestructableLife(target) : wc3::GetUnitState(target,0));};
    float before=life(),base=wc3::UnitAttackAverage(actor)*tuning.damageMultiplier;
    bool allied=!destructable && wc3::IsUnitAlly(target,wc3::GetOwningPlayer(actor));
    float damage=CombatDamage::Amount(base,allied,false,before,settings.friendlyFirePercent);
    float distance=destructable ? nearest : std::hypot(wc3::Real(wc3::GetUnitX(target))-x,wc3::Real(wc3::GetUnitY(target))-y);
    bool effects=damage>0 && !destructable && settings.enableAttackEffects && NativeWeaponAttack::Apply(actor,target,damage,distance);
    if(damage>0 && !effects)wc3::UnitDamageTarget(actor,target,&damage,TRUE,FALSE,NativeWeaponAttack::AttackType(actor),4,0);
    float after=life();
    wc3::Log("Third-person hit target=%08X hpBefore=%.1f hpAfter=%.1f damage=%.1f allied=%d effects=%d",target,before,after,damage,allied,effects);
    return after<before;
}
