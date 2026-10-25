#pragma once
#include "WarcraftApi.hpp"
#include <functional>

// Capture hostility and location before map death triggers remove or transfer the victim.
struct KillReward {
    using Sink=std::function<void(const KillReward&)>;
    wc3::Handle owner=0,target=0;
    float x=0,y=0;
    static KillReward Capture(wc3::Handle actor,wc3::Handle victim,const Sink& sink) {
        if(!sink || !wc3::GetUnitTypeId(actor) || !wc3::GetUnitTypeId(victim) ||
            wc3::Real(wc3::GetUnitState(victim,0))<=.405f)return {};
        auto owner=wc3::GetOwningPlayer(actor);
        if(!wc3::IsUnitEnemy(victim,owner) || wc3::IsUnitType(victim,2))return {};
        return {owner,victim,wc3::Real(wc3::GetUnitX(victim)),wc3::Real(wc3::GetUnitY(victim))};
    }
    void Confirm(const Sink& sink) const {
        // Nonlethal hits and repeated hits on corpses cannot award money or reroll a drop.
        if(target && sink && (!wc3::GetUnitTypeId(target) || wc3::Real(wc3::GetUnitState(target,0))<=.405f))sink(*this);
    }
};
