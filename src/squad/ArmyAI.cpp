#include "ArmyAI.hpp"
#include "../platform/FpsCombatGuard.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr int Stop=851972,Attack=851983;
bool Idle(wc3::Handle u) {int order=wc3::GetUnitCurrentOrder(u);return !order || order==Stop;}
bool AliveOwned(wc3::Handle u,wc3::Handle owner) {
    return u && wc3::GetUnitTypeId(u) && wc3::GetOwningPlayer(u)==owner && wc3::Real(wc3::GetUnitState(u,0))>.405f;
}
bool Available(wc3::Handle u,wc3::Handle owner) {
    return AliveOwned(u,owner) && !wc3::IsUnitHidden(u) && !wc3::IsUnitLoaded(u) && !wc3::IsUnitPaused(u);
}
}
void ArmyAI::Tick(wc3::Handle actor,const ArmyAISettings& s,DWORD now,const std::function<bool(wc3::Handle)>& reserved) {
    if(!active_ || !s.enabled || !actor || (next_ && int(now-next_)<0))return;
    next_=now+s.intervalMs;
    auto owner=wc3::GetLocalPlayer();
    // A campaign can transfer the actor between frames; its new owner's army is never ours to automate.
    if(!Available(actor,owner)){status_=ArmyAIStatus::Unavailable;return;}
    std::vector<wc3::Handle> all,free;
    // Order events may run map scripts synchronously. Recheck eligibility before each subsequent command.
    auto eligible=[&](wc3::Handle u) {return u!=actor && Available(actor,owner) && Available(u,owner) && !reserved(u);};
    auto group=wc3::CreateGroup();wc3::GroupEnumUnitsOfPlayer(group,owner,0);
    while(auto u=wc3::FirstOfGroup(group)) {
        wc3::GroupRemoveUnit(group,u);if(!AliveOwned(u,owner))continue;all.push_back(u);
        // Mining workers and garrisons still count toward production limits while unavailable for new orders.
        if(u!=actor && Available(u,owner) && !reserved(u))free.push_back(u);
    }
    wc3::DestroyGroup(group);
    for(auto it=travel_.begin();it!=travel_.end();) {
        if(std::find(free.begin(),free.end(),it->first)==free.end() || wc3::UnitObject(it->first)!=it->second.identity)it=travel_.erase(it);else ++it;
    }
    ArmyPlan plan=ArmyPlan::Custom(s);
    if(!plan.Complete()) {std::vector<int> types;for(auto u:all)types.push_back(wc3::GetUnitTypeId(u));plan=ArmyPlan::Detect(types);}
    float bx=wc3::Real(wc3::GetUnitX(actor)),by=wc3::Real(wc3::GetUnitY(actor));
    auto base=std::find_if(all.begin(),all.end(),[&](wc3::Handle u){return plan.Hall(wc3::GetUnitTypeId(u));});
    if(base!=all.end()) {bx=wc3::Real(wc3::GetUnitX(*base));by=wc3::Real(wc3::GetUnitY(*base));}
    // A distant controlled hero is not a construction site when the campaign supplies workers elsewhere.
    if(base==all.end())for(auto u:free)if(wc3::GetUnitTypeId(u)==plan.worker){bx=wc3::Real(wc3::GetUnitX(u));by=wc3::Real(wc3::GetUnitY(u));break;}
    std::vector<wc3::Handle> lumberGhouls;
    unsigned commands=0,attempts=0;bool resourceAvailable=false,workerAvailable=false;
    auto count=[&](int type) {return int(std::count_if(all.begin(),all.end(),[&](wc3::Handle u){return wc3::GetUnitTypeId(u)==type;}));};
    // Native GetUnitCurrentOrder can be zero during training. Reserve accepted production until population grows.
    // A decision clock advances only while this assistant can run, so a long pause cannot expire every reservation.
    productionTime_+=s.intervalMs;
    for(auto it=training_.begin();it!=training_.end();) {
        if(!AliveOwned(it->first,owner) || wc3::UnitObject(it->first)!=it->second.identity || int(productionTime_-it->second.until)>=0)it=training_.erase(it);else ++it;
    }
    for(int type:{plan.worker,plan.soldier})if(type) {
        int current=count(type);auto previous=populations_.find(type);
        int completed=previous==populations_.end() ? 0 : std::max(0,current-previous->second);
        for(auto it=training_.begin();completed && it!=training_.end();) {
            if(it->second.type==type) {it=training_.erase(it);--completed;}else ++it;
        }
        populations_[type]=current;
    }
    for(auto it=pending_.begin();it!=pending_.end();) {
        if(count(it->first)>it->second.count || int(now-it->second.until)>=0)it=pending_.erase(it);else ++it;
    }
    // Construction/training requests are real orders: native tech limits, gold, lumber, food and map triggers decide success.
    if(plan.Complete()) {
        // Gold workers need a race-compatible mine. Enemy mines never satisfy the economy.
        wc3::Handle mine=0,rawMine=0;float search=s.resourceRadius,nearest=search,rawDistance=search;
        group=wc3::CreateGroup();wc3::GroupEnumUnitsInRange(group,&bx,&by,&search,0);
        while(auto u=wc3::FirstOfGroup(group)) {
            wc3::GroupRemoveUnit(group,u);int type=wc3::GetUnitTypeId(u);
            if(wc3::Real(wc3::GetUnitState(u,0))<=.405f)continue;
            float d=std::hypot(wc3::Real(wc3::GetUnitX(u))-bx,wc3::Real(wc3::GetUnitY(u))-by);
            if(type==ArmyPlan::Code("ngol") && d<rawDistance){rawMine=u;rawDistance=d;}
            bool compatible=plan.worker==ArmyPlan::Code("uaco") ? type==ArmyPlan::Code("ugol") && wc3::GetOwningPlayer(u)==owner :
                plan.worker==ArmyPlan::Code("ewsp") ? type==ArmyPlan::Code("egol") && wc3::GetOwningPlayer(u)==owner : type==ArmyPlan::Code("ngol");
            if(compatible && d<nearest){mine=u;nearest=d;}
        }
        wc3::DestroyGroup(group);resourceAvailable=mine || rawMine;
        // Night elf wisps cannot mine an unentangled mine; use the hall's real ability when the map permits it.
        if(plan.worker==ArmyPlan::Code("ewsp") && !mine && rawMine && base!=all.end() && eligible(*base) && Idle(*base) &&
            wc3::GetUnitAbilityLevel(*base,ArmyPlan::Code("Aent"))>0 && wc3::IssueTargetOrderById(*base,wc3::OrderId("entangle"),rawMine))++commands;
        int need=base==all.end() ? plan.hall : plan.worker==ArmyPlan::Code("uaco") && !mine && rawMine ? ArmyPlan::Code("ugol") : count(plan.supply)<s.supplyBuildings &&
            wc3::GetPlayerState(owner,5)-wc3::GetPlayerState(owner,4)<4 ? plan.supply : !count(plan.barracks) ? plan.barracks : 0;
        if(need && !pending_.count(need))for(auto worker:free) {
            if(!eligible(worker) || wc3::GetUnitTypeId(worker)!=plan.worker || FpsCombatGuard::AttackTargetObject(worker))continue;
            int order=wc3::GetUnitCurrentOrder(worker);
            if(!Idle(worker) && order!=wc3::OrderId("harvest") && order!=wc3::OrderId("returnresources"))continue;
            workerAvailable=true;
            // A blocked site must not stop development for the whole decision. Bound total native attempts.
            while(attempts<s.placementAttempts && eligible(worker)) {
                ++attempts;float angle=float(placement_++%16)*.3926991f;
                float distance=s.buildRadius*(.6f+.2f*float((placement_/16)%3));
                float x=bx+std::cos(angle)*distance,y=by+std::sin(angle)*distance;
                if(need==ArmyPlan::Code("ugol") && rawMine){x=wc3::Real(wc3::GetUnitX(rawMine));y=wc3::Real(wc3::GetUnitY(rawMine));}
                BOOL accepted=wc3::IssueBuildOrderById(worker,need,&x,&y);
                wc3::Log("army AI build worker=%08X type=%08X accepted=%d x=%.1f y=%.1f",worker,need,accepted,x,y);
                if(accepted) {pending_[need]={now+60000,count(need)};++commands;break;}
                if(need==ArmyPlan::Code("ugol"))break; // One actual mine, not arbitrary build coordinates.
            }
            if(pending_.count(need) || attempts>=s.placementAttempts)break;
        }
        int workers=count(plan.worker),soldiers=count(plan.soldier);
        for(const auto& p:training_) {if(p.second.type==plan.worker)++workers;if(p.second.type==plan.soldier)++soldiers;}
        for(auto u:all) {
            if(training_.count(u))continue; // Do not count one accepted queue both as a lease and as a visible native order.
            int order=wc3::GetUnitCurrentOrder(u);if(order==plan.worker)++workers;if(order==plan.soldier)++soldiers;
        }
        for(auto building:free) {
            if(!eligible(building) || !Idle(building) || training_.count(building))continue;int type=wc3::GetUnitTypeId(building),train=0;
            if(plan.Hall(type) && workers<s.workers)train=plan.worker;
            if(type==plan.barracks && soldiers<s.soldiers)train=plan.soldier;
            if(train && wc3::IssueImmediateOrderById(building,train)) {
                training_[building]={wc3::UnitObject(building),train,productionTime_+s.productionRetrySeconds*1000};++commands;
                if(train==plan.worker)++workers;else ++soldiers;
                wc3::Log("army AI train building=%08X type=%08X",building,train);
            }
        }
        // Idle stock workers gather through their own race's native abilities; unsupported custom resources remain untouched.
        auto trees=wc3::NearbyDestructables(bx,by,search);resourceAvailable|=!trees.empty();
        // Acolytes cannot harvest lumber. Count returning ghouls too, or every delivery could recruit more lumber workers.
        if(plan.worker==ArmyPlan::Code("uaco")) {
            for(auto ghoul:free)if(eligible(ghoul) && wc3::GetUnitTypeId(ghoul)==ArmyPlan::Code("ugho") &&
                (wc3::GetUnitCurrentOrder(ghoul)==wc3::OrderId("harvest") || wc3::GetUnitCurrentOrder(ghoul)==wc3::OrderId("returnresources")))lumberGhouls.push_back(ghoul);
            for(auto ghoul:free) {
                if(int(lumberGhouls.size())>=s.lumberWorkers)break;
                if(!eligible(ghoul) || wc3::GetUnitTypeId(ghoul)!=ArmyPlan::Code("ugho") || !Idle(ghoul) || FpsCombatGuard::AttackTargetObject(ghoul))continue;
                unsigned tried=0;
                for(auto tree:trees) {
                    if(++tried>64 || !eligible(ghoul))break;
                    if(wc3::Real(wc3::GetDestructableLife(tree))>.405f && wc3::IssueTargetOrderById(ghoul,wc3::OrderId("harvest"),tree)) {
                        lumberGhouls.push_back(ghoul);++commands;break;
                    }
                }
            }
        }
        unsigned index=0;
        for(auto worker:free)if(eligible(worker) && wc3::GetUnitTypeId(worker)==plan.worker && Idle(worker)) {
            workerAvailable=true;bool lumber=index++%3 && plan.worker!=ArmyPlan::Code("uaco");bool accepted=false;
            if(!lumber && mine)accepted=wc3::IssueTargetOrderById(worker,wc3::OrderId("harvest"),mine)!=FALSE;
            // A gate near a tree cannot monopolize selection: try bounded native-valid harvest targets.
            unsigned tried=0;
            if(!accepted && plan.worker!=ArmyPlan::Code("uaco"))for(auto tree:trees) {
                if(++tried>64)break;
                if(wc3::Real(wc3::GetDestructableLife(tree))>.405f && wc3::IssueTargetOrderById(worker,wc3::OrderId("harvest"),tree)) {accepted=true;break;}
            }
            if(!accepted && mine)accepted=wc3::IssueTargetOrderById(worker,wc3::OrderId("harvest"),mine)!=FALSE;
            if(accepted)++commands;
        }
    }
    // A visible reason distinguishes native campaign limitations from an unresponsive U key.
    if(!plan.Complete())status_=ArmyAIStatus::NoEconomy;
    else if(commands)status_=ArmyAIStatus::Working;
    else if(!pending_.empty() || !training_.empty())status_=ArmyAIStatus::Waiting;
    else if(!count(plan.worker))status_=ArmyAIStatus::NoWorkers;
    else if(count(plan.worker)>=s.workers && count(plan.soldier)>=s.soldiers && base!=all.end() && count(plan.barracks))status_=ArmyAIStatus::Ready;
    else if(!workerAvailable)status_=ArmyAIStatus::WorkersBusy;
    else if(!resourceAvailable)status_=ArmyAIStatus::NoResources;
    else status_=ArmyAIStatus::Blocked;
    // Defend threats independently of optional hero escort; reserves return only to a real base.
    wc3::Handle threat=0;float search=s.baseRadius;
    group=wc3::CreateGroup();wc3::GroupEnumUnitsInRange(group,&bx,&by,&search,0);
    while(auto u=wc3::FirstOfGroup(group)) {
        wc3::GroupRemoveUnit(group,u);
        if(wc3::IsUnitEnemy(u,owner) && wc3::IsUnitVisible(u,owner) && !wc3::IsUnitHidden(u) && wc3::Real(wc3::GetUnitState(u,0))>.405f) {threat=u;break;}
    }
    wc3::DestroyGroup(group);int defenders=0;
    for(auto u:free) {
        if(!eligible(u) || std::find(lumberGhouls.begin(),lumberGhouls.end(),u)!=lumberGhouls.end())continue;
        int type=wc3::GetUnitTypeId(u);
        bool worker=type==plan.worker;
        for(const auto& race:ArmyPlan::Races())worker|=type==race.worker;
        uintptr_t object=wc3::UnitObject(u);
        if(worker || wc3::IsUnitType(u,2) || wc3::Real(wc3::GetUnitMoveSpeed(u))<=0 || !object ||
            !*reinterpret_cast<uintptr_t*>(object+0x1E8) || FpsCombatGuard::AttackTargetObject(u))continue;
        bool defense=defenders++<s.defenders;
        auto own=travel_.find(u);int order=wc3::GetUnitCurrentOrder(u);
        if(!Idle(u) && (own==travel_.end() || order!=Attack))continue; // Never replace manual/spell/mission orders.
        if(threat && (!wc3::GetUnitTypeId(threat) || !wc3::IsUnitEnemy(threat,owner) || !wc3::IsUnitVisible(threat,owner) ||
            wc3::Real(wc3::GetUnitState(threat,0))<=.405f))threat=0;
        if(threat) {
            if(wc3::IssueTargetOrderById(u,Attack,threat)) {
                travel_.erase(u);wc3::Log("army AI defend unit=%08X target=%08X",u,threat);
            }
            continue;
        }
        // No hall means no base rally either: campaign troops must not silently become hero followers.
        if(!s.followActor && (!defense || base==all.end())) {
            // A live opt-out releases only our outstanding approach, after manual/combat exclusions above.
            if(own!=travel_.end() && order==Attack) {wc3::IssueImmediateOrderById(u,Stop);travel_.erase(own);}
            continue;
        }
        float x=defense ? bx : wc3::Real(wc3::GetUnitX(actor)),y=defense ? by : wc3::Real(wc3::GetUnitY(actor));
        if(std::hypot(wc3::Real(wc3::GetUnitX(u))-x,wc3::Real(wc3::GetUnitY(u))-y)<=s.supportDistance) {
            // Stop only our idle approach before the unit crowds the actor; native combat was checked above.
            if(own!=travel_.end() && order==Attack) {wc3::IssueImmediateOrderById(u,Stop);travel_.erase(own);}
            continue;
        }
        if(own!=travel_.end() && order==Attack && std::hypot(own->second.x-x,own->second.y-y)<s.supportDistance*.5f)continue;
        if(wc3::IssuePointOrderById(u,Attack,&x,&y)) {
            travel_[u]={object,x,y};wc3::Log("army AI travel unit=%08X type=%08X defense=%d x=%.1f y=%.1f",u,type,defense,x,y);
        }
    }
}
