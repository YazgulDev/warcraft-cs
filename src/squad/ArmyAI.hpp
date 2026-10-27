#pragma once
#include "ArmyPlan.hpp"
#include "ArmyAIStatus.hpp"
#include "../platform/WarcraftApi.hpp"
#include <functional>
#include <map>

// A bounded offline assistant owns only its issued orders, never unit ownership or resource counters.
class ArmyAI {
public:
    bool Toggle() {active_=!active_;next_=0;status_=active_ ? ArmyAIStatus::Scanning : ArmyAIStatus::Off;return active_;}
    ArmyAIStatus Status() const {return status_;}
    bool Active() const {return active_;}
    void Reset() {status_=ArmyAIStatus::Off;active_=false;next_=0;pending_.clear();travel_.clear();training_.clear();populations_.clear();productionTime_=0;placement_=0;}
    void Tick(wc3::Handle actor,const ArmyAISettings& settings,DWORD now,const std::function<bool(wc3::Handle)>& reserved);
private:
    struct Travel {uintptr_t identity=0;float x=0,y=0;};
    ArmyAIStatus status_=ArmyAIStatus::Off;
    bool active_=false;
    DWORD next_=0;
    unsigned placement_=0;
    struct Pending {DWORD until;int count;};
    std::map<int,Pending> pending_;
    std::map<wc3::Handle,Travel> travel_;
    struct Training {uintptr_t identity;int type;DWORD until;};
    std::map<wc3::Handle,Training> training_;
    std::map<int,int> populations_;
    DWORD productionTime_=0;
};
