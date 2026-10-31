#include "NativeNuclearDestruction.hpp"
#include <algorithm>
#include <cmath>

void NativeNuclearDestruction::Apply(wc3::Handle actor,int type,uintptr_t object,wc3::Handle owner,const GrenadePoint& center,const NuclearSettings& settings,
    const std::function<bool()>& generationValid,const KillReward::Sink& rewards){
    auto valid=[&]{return generationValid() && wc3::GetUnitTypeId(actor)==type && wc3::UnitObject(actor)==object && wc3::GetOwningPlayer(actor)==owner;};
    if(!valid())return;struct Victim{wc3::Handle unit;int type;uintptr_t object;};std::vector<Victim> victims;
    auto group=wc3::CreateGroup();if(!group)return;float x=center[0],y=center[1],radius=settings.radius;
    wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    for(auto unit=wc3::FirstOfGroup(group);unit;unit=wc3::FirstOfGroup(group)){wc3::GroupRemoveUnit(group,unit);victims.push_back({unit,wc3::GetUnitTypeId(unit),wc3::UnitObject(unit)});}wc3::DestroyGroup(group);
    // Copy the destructable list before any native death trigger can change pathing or unload the map.
    std::vector<Victim> destructables;
    if(settings.destructables)for(auto target:wc3::NearbyDestructables(x,y,radius))destructables.push_back({target,wc3::GetDestructableTypeId(target),wc3::DestructableObject(target)});
    unsigned killed=0,scenery=0,protectedUnits=0;
    for(const auto& victim:victims){
        if(!valid())return;auto target=victim.unit;
        if(target==actor || !victim.type || wc3::GetUnitTypeId(target)!=victim.type || wc3::UnitObject(target)!=victim.object || wc3::IsUnitHidden(target) || wc3::IsUnitLoaded(target))continue;
        bool neutral=wc3::IsNeutralUnit(target),building=wc3::IsUnitType(target,2)!=FALSE;
        bool neutralTarget=neutral && (building ? settings.neutralBuildings : settings.neutralUnits);
        bool hostile=wc3::IsUnitEnemy(target,owner) && !wc3::IsUnitAlly(target,owner) && (!building || settings.buildings);
        if(!neutralTarget && !hostile)continue;
        if(std::hypot(wc3::Real(wc3::GetUnitX(target))-x,wc3::Real(wc3::GetUnitY(target))-y)>radius)continue;
        float before=wc3::Real(wc3::GetUnitState(target,0));if(!std::isfinite(before) || before<=.405f)continue;
        bool forcedNeutralBuilding=neutralTarget && building;
        // Neutral shops often have Avul. Their explicit requested destruction uses native death, not RemoveUnit.
        if(wc3::GetUnitAbilityLevel(target,0x4176756C)>0 && !forcedNeutralBuilding){++protectedUnits;continue;}
        auto sink=settings.rewards ? rewards : KillReward::Sink{};auto reward=KillReward::Capture(actor,target,sink);
        float maximum=wc3::Real(wc3::GetUnitState(target,1));if(!std::isfinite(maximum))maximum=before;
        float damage=std::clamp(std::max(before,maximum)*4.f,1000000.f,1e12f);wc3::UnitDamageTarget(actor,target,&damage,FALSE,TRUE,5,26,0);
        if(!valid())return;
        if(forcedNeutralBuilding && wc3::GetUnitTypeId(target)==victim.type && wc3::UnitObject(target)==victim.object && wc3::IsNeutralUnit(target) && wc3::Real(wc3::GetUnitState(target,0))>.405f)wc3::KillUnit(target);
        if(!valid())return;reward.Confirm(sink);if(!valid())return;
        float after=wc3::GetUnitTypeId(target) ? wc3::Real(wc3::GetUnitState(target,0)) : 0;if(after<=.405f)++killed;else ++protectedUnits;
        wc3::Log("Nuclear impact unit=%08X neutral=%d building=%d hpBefore=%.1f hpAfter=%.1f",target,neutral,building,before,after);
    }
    for(const auto& victim:destructables){
        if(!valid())return;auto target=victim.unit;
        if(!victim.type || wc3::GetDestructableTypeId(target)!=victim.type || wc3::DestructableObject(target)!=victim.object || wc3::Real(wc3::GetDestructableLife(target))<=.405f)continue;
        if(std::hypot(wc3::Real(wc3::GetDestructableX(target))-x,wc3::Real(wc3::GetDestructableY(target))-y)>radius)continue;
        // Engine death removes tree/gate pathing and dispatches native destructable death events.
        wc3::KillDestructable(target);if(!valid())return;
        if(wc3::GetDestructableTypeId(target)!=victim.type || wc3::DestructableObject(target)!=victim.object || wc3::Real(wc3::GetDestructableLife(target))<=.405f)++scenery;
    }
    wc3::Log("Nuclear impact destroyed units=%u destructables=%u protected=%u radius=%.0f",killed,scenery,protectedUnits,radius);
}
