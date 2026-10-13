#pragma once
#include "WarcraftApi.hpp"
#include "../presentation/HeroAttackPose.hpp"
#include "../presentation/ModelMotionProfile.hpp"

// Own manual attack pose/rate only; contact and cooldown stay in UnitAttack's combat timeline.
class UnitAttackAnimation {
public:
    void Begin(wc3::Handle unit,double now,float attackSpeed,float contact,float interval);
    void Update(double now);
    bool Active(double now) const {return pose_.Active(now);}
    void Release();
    void Reset() {unit_=0;type_=0;owned_=false;sequence_=-1;pose_.Reset();path_.clear();}
private:
    wc3::Handle unit_=0;
    int type_=0,sequence_=-1;
    bool owned_=false;
    float baseline_=1;
    std::string path_;
    ModelMotionProfile profile_;
    HeroAttackPose pose_;
};
