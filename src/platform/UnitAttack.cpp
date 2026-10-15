#include "UnitAttack.hpp"
#include "NativeWeaponAttack.hpp"
#include "WeaponTargetFilter.hpp"
#include "../combat/CombatDamage.hpp"
#include <cmath>

bool UnitAttack::Supported(wc3::Handle unit) {
    // Accept native melee/ranged primary attacks, including map-defined heroes; unarmed workers remain unsupported.
    return NativeWeaponAttack::PrimaryKind(unit)!=0;
}
bool UnitAttack::Begin(wc3::Handle unit,double now,const GameplaySettings& settings,const UnitStatus& status) {
    if(status.disarmed || status.incapacitated || !Supported(unit) ||
        !timeline_.Begin(now,settings.thirdPerson.contactDelay,settings.thirdPerson.attackInterval,status.attackSpeed))return false;
    animation_.Begin(unit,now,status.attackSpeed,settings.thirdPerson.contactDelay,settings.thirdPerson.attackInterval);
    wc3::Log("Third-person swing unit=%08X attack=%.1f speed=%.2f",unit,status.attackDamage,status.attackSpeed);return true;
}
bool UnitAttack::Tick(wc3::Handle unit,double now,float yaw,float feet,const float* aimOrigin,const float* aimDirection,bool casting,
    const GameplaySettings& settings,const UnitStatus& status,UnitHitboxes& units,DestructableHitboxes& destructables) {
    if(casting || status.incapacitated || status.disarmed || status.hidden || status.contained) {
        Cancel();return false;
    }
    // Locomotion owns stand/run/jump poses separately; contact never overwrites native spell animation.
    animation_.Update(now);
    if(!timeline_.Contact(now))return false;
    auto target=Preview(unit,yaw,feet,aimOrigin,aimDirection,settings,units,destructables);
    if(!target.handle) {
        // Keep one miss diagnostic per manual contact; idle prediction must not flood the journal.
        wc3::Log("Third-person %s miss unit=%08X",NativeWeaponAttack::PrimaryKind(unit)==1 ? "melee" : "ranged",unit);
        return false;
    }
    bool hit=Damage(unit,target.handle,target.destructable,target.distance,NativeWeaponAttack::PrimaryKind(unit)!=1,settings);
    // Native impact cleanup can reset the sprite in this same rendered frame; retain the current backswing time.
    animation_.Update(now);return hit;
}
UnitAimTarget UnitAttack::Preview(wc3::Handle unit,float yaw,float feet,const float* origin,const float* direction,
    const GameplaySettings& settings,UnitHitboxes& units,DestructableHitboxes& destructables) {
    // Prediction has no animation/order/damage side effects and shares the exact contact policy with LMB.
    int kind=NativeWeaponAttack::PrimaryKind(unit);if(!kind)return {};
    auto target=kind==1 ? PickMelee(unit,yaw,feet,settings,units,destructables) :
        PickRanged(unit,feet,origin,direction,settings,units,destructables);
    target.allied=target.handle && !target.destructable && wc3::IsUnitAlly(target.handle,wc3::GetOwningPlayer(unit));
    // Recheck allegiance after selection; protected allies are already skipped by both pickers.
    if(target.allied && !settings.targetAllies)return {};
    if(target.handle) {
        // Only actual enemies get red. Neutral/passive, allied and destructable contacts use yellow.
        target.enemy=!target.destructable && wc3::IsUnitEnemy(target.handle,wc3::GetOwningPlayer(unit));
        float x=wc3::Real(target.destructable ? wc3::GetDestructableX(target.handle) : wc3::GetUnitX(target.handle));
        float y=wc3::Real(target.destructable ? wc3::GetDestructableY(target.handle) : wc3::GetUnitY(target.handle));
        for(size_t i=0;i<target.circle.size();++i) {
            float angle=float(i)*6.28318530718f/target.circle.size();
            float px=x+std::cos(angle)*settings.thirdPerson.targetCircleRadius;
            float py=y+std::sin(angle)*settings.thirdPerson.targetCircleRadius;
            target.circle[i]={px,py,wc3::Ground(px,py)+3}; // Lift the decal just above terrain/bridge decks.
        }
    }
    return target;
}
UnitAimTarget UnitAttack::PickMelee(wc3::Handle actor,float yaw,float feet,const GameplaySettings& settings,
    UnitHitboxes& units,DestructableHitboxes& destructables) {
    const auto& tuning=settings.thirdPerson;
    float origin[]={wc3::Real(wc3::GetUnitX(actor)),wc3::Real(wc3::GetUnitY(actor)),feet+tuning.attackHeight};
    float x=origin[0],y=origin[1],radius=tuning.attackRange+512,nearest=tuning.attackRange;
    auto group=wc3::CreateGroup();wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    std::vector<wc3::Handle> candidates;
    while(auto unit=wc3::FirstOfGroup(group)) {wc3::GroupRemoveUnit(group,unit);candidates.push_back(unit);}
    wc3::DestroyGroup(group);
    auto objects=wc3::NearbyDestructables(x,y,radius);
    wc3::Handle target=0;bool destructable=false;float point[3]={};
    // Sweep the authored forward arc. Each sample competes with terrain/gates, so a side ray cannot pierce a wall.
    for(int height=-1;height<=1;++height)for(int ray=0;ray<=10;++ray) {
        // Short creeps should not slip beneath one waist-height ray. Each layer still respects terrain/solid bodies.
        origin[2]=feet+std::max(1.f,tuning.attackHeight+height*tuning.meleeHeightTolerance);
        float angle=(yaw-tuning.attackArc*.5f+tuning.attackArc*ray/10)*.01745329252f;
        float direction[]={std::cos(angle),std::sin(angle),0},limit=tuning.attackRange;
        // Share FPS's solid-surface policy, including low melee samples below the water plane.
        for(float t=8;t<=limit;t+=8)if(wc3::AttackSurface(x+direction[0]*t,y+direction[1]*t,settings.waterBlocksAttacks)>=origin[2]) {limit=t;break;}
        wc3::Handle contact=0;bool isObject=false;
        for(auto unit:candidates) {
            if(unit==actor || wc3::IsUnitHidden(unit) || wc3::IsUnitLoaded(unit) ||
                wc3::Real(wc3::GetUnitState(unit,0))<=.405f || !wc3::IsUnitVisible(unit,wc3::GetLocalPlayer()))continue;
            // Skip protected allies before shortening any sweep ray; they cannot hide the enemy behind them.
            if(!settings.targetAllies && wc3::IsUnitAlly(unit,wc3::GetOwningPlayer(actor)))continue;
            if(settings.ignorePassableInvulnerableObjects && WeaponTargetFilter::PassThrough(unit,
                reinterpret_cast<uintptr_t>(GetModuleHandleA("Game.dll"))))continue;
            float entry=0;if(units.Intersect(unit,origin,direction,limit,entry)) {limit=entry;contact=unit;isObject=false;}
        }
        for(auto object:objects) {
            float entry=0;if(wc3::Real(wc3::GetDestructableLife(object))>.405f &&
                destructables.Intersect(object,origin,direction,limit,entry)) {limit=entry;contact=object;isObject=true;}
        }
        if(contact && limit<=nearest) {
            nearest=limit;target=contact;destructable=isObject;
            for(int i=0;i<3;++i)point[i]=origin[i]+direction[i]*limit;
        }
    }
    if(!target)return {};
    float distance=destructable ? nearest : std::hypot(wc3::Real(wc3::GetUnitX(target))-x,wc3::Real(wc3::GetUnitY(target))-y);
    return {target,destructable,false,distance,{point[0],point[1],point[2]}};
}
bool UnitAttack::Damage(wc3::Handle actor,wc3::Handle target,bool destructable,float distance,bool ranged,const GameplaySettings& settings) {
    const auto& tuning=settings.thirdPerson;
    auto life=[&] {return wc3::Real(destructable ? wc3::GetDestructableLife(target) : wc3::GetUnitState(target,0));};
    float before=life(),base=wc3::UnitAttackAverage(actor)*tuning.damageMultiplier;
    bool allied=!destructable && wc3::IsUnitAlly(target,wc3::GetOwningPlayer(actor));
    if(allied && !settings.targetAllies)return false; // Recheck at dispatch, preserving alliance changes.
    float damage=CombatDamage::Amount(base,allied,false,before,settings.friendlyFirePercent);
    bool projectile=ranged && tuning.nativeRangedProjectiles;
    // Warcraft rolls native attack effects and creates its missile; non-native contacts have an explicit instant fallback.
    bool effects=damage>0 && !destructable && settings.enableAttackEffects && (projectile ?
        NativeWeaponAttack::Launch(actor,target,damage,distance) : NativeWeaponAttack::Apply(actor,target,damage,distance));
    if(damage>0 && !effects)wc3::UnitDamageTarget(actor,target,&damage,TRUE,FALSE,NativeWeaponAttack::AttackType(actor),4,0);
    float after=life();
    wc3::Log("Third-person hit target=%08X hpBefore=%.1f hpAfter=%.1f damage=%.1f allied=%d effects=%d ranged=%d projectile=%d",target,before,after,damage,allied,effects,ranged,projectile && effects);
    return after<before;
}
UnitAimTarget UnitAttack::PickRanged(wc3::Handle actor,float feet,const float* aimOrigin,const float* aimDirection,
    const GameplaySettings& settings,UnitHitboxes& units,DestructableHitboxes& destructables) {
    const auto& tuning=settings.thirdPerson;
    float range=tuning.useNativeAttackRange ? NativeWeaponAttack::PrimaryRange(actor) : 0;
    range=std::clamp((range>0 ? range : tuning.rangedAttackRange)*tuning.rangedRangeMultiplier,30.f,5000.f);
    float muzzle[]={wc3::Real(wc3::GetUnitX(actor)),wc3::Real(wc3::GetUnitY(actor)),feet+tuning.attackHeight};
    float x=muzzle[0],y=muzzle[1],radius=range+512;
    std::vector<wc3::Handle> candidates;
    auto group=wc3::CreateGroup();wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    while(auto unit=wc3::FirstOfGroup(group)) {
        wc3::GroupRemoveUnit(group,unit);
        if(unit!=actor && !wc3::IsUnitHidden(unit) && !wc3::IsUnitLoaded(unit) &&
            wc3::Real(wc3::GetUnitState(unit,0))>.405f && wc3::IsUnitVisible(unit,wc3::GetLocalPlayer()) &&
            // The reticle and muzzle traces share one filtered list, including allied buildings.
            (settings.targetAllies || !wc3::IsUnitAlly(unit,wc3::GetOwningPlayer(actor))) &&
            !(settings.ignorePassableInvulnerableObjects && WeaponTargetFilter::PassThrough(unit,
                reinterpret_cast<uintptr_t>(GetModuleHandleA("Game.dll")))))candidates.push_back(unit);
    }
    wc3::DestroyGroup(group);
    auto objects=wc3::NearbyDestructables(x,y,radius);
    // Both the reticle trace and the body-to-contact trace obey the same nearest-body/terrain rule.
    auto trace=[&](const float* origin,const float* direction,float& limit,bool& destructable) {
        wc3::Handle contact=0;destructable=false;
        for(float t=0;t<=limit;t+=8)if(wc3::AttackSurface(origin[0]+direction[0]*t,origin[1]+direction[1]*t,settings.waterBlocksAttacks)>=origin[2]+direction[2]*t){limit=t;break;}
        for(auto unit:candidates){float entry=0;if(units.Intersect(unit,origin,direction,limit,entry)){limit=entry;contact=unit;destructable=false;}}
        for(auto object:objects){float entry=0;if(wc3::Real(wc3::GetDestructableLife(object))>.405f &&
            destructables.Intersect(object,origin,direction,limit,entry)){limit=entry;contact=object;destructable=true;}}
        return contact;
    };
    // Ignore the camera boom behind the actor. Then converge the muzzle on the visible reticle contact to handle parallax.
    float advance=0;for(int i=0;i<3;++i)advance+=(muzzle[i]-aimOrigin[i])*aimDirection[i];
    advance=std::max(0.f,advance);
    float origin[3],point[3],direction[3];for(int i=0;i<3;++i)origin[i]=aimOrigin[i]+aimDirection[i]*advance;
    float limit=range;bool object=false;trace(origin,aimDirection,limit,object);
    for(int i=0;i<3;++i){point[i]=origin[i]+aimDirection[i]*limit;direction[i]=point[i]-muzzle[i];}
    float length=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2]);
    if(length<.001f)return {};
    for(auto& component:direction)component/=length;
    // A small extension crosses the picked surface instead of missing it by float rounding; the native range is still capped.
    limit=std::min(range,length+1.f);auto target=trace(muzzle,direction,limit,object);
    if(!target)return {};
    float distance=object ? limit : std::hypot(wc3::Real(wc3::GetUnitX(target))-x,wc3::Real(wc3::GetUnitY(target))-y);
    for(int i=0;i<3;++i)point[i]=muzzle[i]+direction[i]*limit;
    return {target,object,false,distance,{point[0],point[1],point[2]}};
}
