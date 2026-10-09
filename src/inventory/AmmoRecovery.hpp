#pragma once
#include "../combat/WeaponSlots.hpp"
#include <algorithm>
#include <cmath>

// Fractional credits preserve small percentages: five 20% pickups restore one C4, not one per pickup.
class AmmoRecovery {
public:
    void Reset() { for (auto& value:magazineCredit_) value=0; for (auto& value:reserveCredit_) value=0; }
    int Restore(int slot,float percent,int capacity,int reserveCapacity,int& magazine,int& reserve) {
        if (slot<0 || slot>=WeaponSlots::Count || !std::isfinite(percent)) return 0;
        percent=std::clamp(percent,0.0f,100.0f);
        int changed=Add(percent,capacity,magazine,magazineCredit_[slot]);
        return changed+Add(percent,reserveCapacity,reserve,reserveCredit_[slot]);
    }
private:
    static int Add(float percent,int capacity,int& amount,double& credit) {
        if (capacity<=0 || amount>=capacity) { credit=0; return 0; }
        credit+=double(capacity)*percent/100;
        int whole=int(std::floor(credit+1e-6));credit-=whole;
        int added=std::min(whole,capacity-amount);amount+=added;
        if (amount>=capacity) credit=0; // Full ammunition cannot bank invisible future rewards.
        return added;
    }
    double magazineCredit_[WeaponSlots::Count]={},reserveCredit_[WeaponSlots::Count]={};
};
