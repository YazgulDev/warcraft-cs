#include "SquadSpellcasting.hpp"
#include "NativeAbilitySlots.hpp"
#include "NativeAbilityInfo.hpp"
#include "MapSpellCharges.hpp"
#include "../combat/UnitStatus.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

void SquadSpellcasting::Configure() {
    base_=reinterpret_cast<uintptr_t>(GetModuleHandleA("Game.dll"));slots_.Configure(base_);
    auto add=[&](SquadSpellRole role,const char* names) {
        std::istringstream list(names);std::string name;
        while(list>>name)if(int order=wc3::OrderId(name.c_str()))rules_[order]=role;
    };
    // Resolve names in Warcraft, so native stock-based map variants share their real order without copied game tables.
    add(SquadSpellRole::Enemy,"thunderbolt firebolt chainlightning frostnova deathcoil entanglingroots shadowstrike slow polymorph hex cripple purge acidbomb soulburn curse forkedlightning");
    add(SquadSpellRole::Point,"blizzard flamestrike rainoffire shockwave carrionswarm impale clusterrockets breathoffire");
    add(SquadSpellRole::Area,"stomp thunderclap fanofknives roar howlofterror");
    add(SquadSpellRole::Heal,"heal healingwave holybolt");
    add(SquadSpellRole::Buff,"bloodlust innerfire frostarmor invisibility lightningshield unholyfrenzy");
    add(SquadSpellRole::Summon,"waterelemental spiritwolf forceofnature summonquillbeast summonhawk summonbear raisedead");
    wc3::Log("squad spell registry orders=%u heal=%d bolt=%d",unsigned(rules_.size()),wc3::OrderId("heal"),wc3::OrderId("thunderbolt"));
}
void SquadSpellcasting::Cancel(wc3::Handle unit) {
    auto found=casts_.find(unit);if(found==casts_.end())return;
    // Stop only a cast this adapter still owns; a later player/map order is never cancelled here.
    if(found->second.order && wc3::UnitObject(unit)==found->second.object && wc3::GetUnitCurrentOrder(unit)==found->second.order)
        wc3::IssueImmediateOrderById(unit,851972);
    casts_.erase(found);
}
bool SquadSpellcasting::Tick(wc3::Handle unit,const GameplaySettings& settings,DWORD now) {
    if(!base_)Configure();if(!base_)return false;
    auto object=wc3::UnitObject(unit);if(!object)return false;
    auto& cast=casts_[unit];if(cast.object!=object){cast={};cast.object=object;}
    int current=wc3::GetUnitCurrentOrder(unit);
    if(cast.order) {
        // Keep approach/windup/channeling intact until native completion, including the initial dispatch window.
        if(current==cast.order || now-cast.requested<100)return true;
        cast.order=0;
    }
    if(current>=852000 && current<=853999)return true; // Preserve externally owned native spell orders too.
    if(cast.attempt && now-cast.attempt<settings.squadSpellIntervalMs)return false;
    cast.attempt=now;auto status=UnitStatus::Read(unit,base_);
    if(status.incapacitated || status.hidden || status.contained)return false;
    float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit)),radius=settings.squadSpellRadius;
    wc3::Handle owner=wc3::GetOwningPlayer(unit),enemy=0,hurt=0;float nearest=radius,lowest=settings.squadHealBelowPercent;
    std::vector<wc3::Handle> allies;
    auto group=wc3::CreateGroup();wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    while(auto candidate=wc3::FirstOfGroup(group)) {
        wc3::GroupRemoveUnit(group,candidate);
        float health=wc3::Real(wc3::GetUnitState(candidate,0));
        if(health<=.405f || wc3::IsUnitHidden(candidate) || wc3::IsUnitLoaded(candidate) || !wc3::IsUnitVisible(candidate,owner))continue;
        float distance=std::hypot(wc3::Real(wc3::GetUnitX(candidate))-x,wc3::Real(wc3::GetUnitY(candidate))-y);
        if(wc3::IsUnitEnemy(candidate,owner) && distance<nearest){nearest=distance;enemy=candidate;}
        if(wc3::IsUnitAlly(candidate,owner)) {
            allies.push_back(candidate);float maximum=wc3::Real(wc3::GetUnitState(candidate,1));
            if(SquadSpellPolicy::NeedsHealing(health,maximum,lowest)){lowest=health*100/maximum;hurt=candidate;}
        }
    }
    wc3::DestroyGroup(group);
    auto abilities=NativeAbilitySlots::All(base_,object);if(abilities.empty())return false;
    for(size_t n=0;n<abilities.size();++n) {
        size_t i=(cast.cursor+n)%abilities.size();int ability=abilities[i],order=slots_.Order(unit,ability);
        if(!rules_.count(order)) {
            // Autocast button getters are dynamic; use verified native classes to choose a manual order,
            // without changing the player's autocast switch or invoking an unknown virtual method.
            auto kind=NativeAbilityInfo::ClassName(slots_.Object(unit,ability));const char* manual=nullptr;
            if(kind==".?AVCAbilityHeal@@")manual="heal";
            else if(kind==".?AVCAbilitySlow@@")manual="slow";
            else if(kind==".?AVCAbilityBloodlust@@")manual="bloodlust";
            else if(kind==".?AVCAbilityInnerFire@@")manual="innerfire";
            else if(kind==".?AVCAbilityCurse@@")manual="curse";
            else if(kind==".?AVCAbilityFrostArmorAuto@@")manual="frostarmor";
            if(manual)order=wc3::OrderId(manual);
        }
        auto rule=rules_.find(order);if(rule==rules_.end())continue;
        wc3::Trace("squad spell candidate unit=%08X ability=%08X order=%d role=%d",unit,ability,order,int(rule->second));
        SpellHudSlot state;state.ability=ability;state.level=wc3::GetUnitAbilityLevel(unit,ability);
        NativeAbilityInfo::Read(slots_.Object(unit,ability),state,wc3::Real(wc3::GetUnitState(unit,2)));
        MapSpellCharges::Read(unit,state); // Preserve recognized map-specific charge limits, like manual casting.
        auto art=state.icon;for(auto& c:art)c=char(std::tolower(static_cast<unsigned char>(c)));
        if(state.charges==0 || art.find("passivebuttons\\")!=std::string::npos || !SquadSpellPolicy::Ready(state.level,state.disabled,state.noMana,state.cooldown))continue;
        auto role=rule->second;bool refresh=role==SquadSpellRole::Buff || role==SquadSpellRole::Summon ||
            (role==SquadSpellRole::Area && state.cooldownTotal<=.05f);
        auto last=cast.refresh.find(ability);
        if(refresh && last!=cast.refresh.end() && now-last->second<DWORD(settings.squadBuffRefreshSeconds*1000))continue;
        BOOL accepted=FALSE;wc3::Handle target=0;
        if(role==SquadSpellRole::Heal && hurt){target=hurt;accepted=wc3::IssueTargetOrderById(unit,order,target);}
        else if(enemy) {
            if(role==SquadSpellRole::Enemy){target=enemy;accepted=wc3::IssueTargetOrderById(unit,order,target);}
            else if(role==SquadSpellRole::Point || role==SquadSpellRole::Summon) {
                float tx=wc3::Real(wc3::GetUnitX(enemy)),ty=wc3::Real(wc3::GetUnitY(enemy));
                accepted=role==SquadSpellRole::Point ? wc3::IssuePointOrderById(unit,order,&tx,&ty) : wc3::IssueImmediateOrderById(unit,order);
                if(!accepted && role==SquadSpellRole::Summon)accepted=wc3::IssuePointOrderById(unit,order,&tx,&ty);
            } else if(role==SquadSpellRole::Area && nearest<=settings.squadAreaSpellRadius)accepted=wc3::IssueImmediateOrderById(unit,order);
            else if(role==SquadSpellRole::Buff)for(auto ally:allies) {
                if(wc3::IssueTargetOrderById(unit,order,ally)){target=ally;accepted=TRUE;break;}
            }
        }
        if(accepted) {
            cast.order=order;cast.requested=now;cast.cursor=(i+1)%abilities.size();if(refresh)cast.refresh[ability]=now;
            wc3::Log("squad spell unit=%08X ability=%08X order=%d target=%08X accepted=1 mana=%.1f",unit,ability,order,target,wc3::Real(wc3::GetUnitState(unit,2)));
            return true;
        }
    }
    return false;
}
