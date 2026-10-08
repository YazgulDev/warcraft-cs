#pragma once
#include "GameplaySettings.hpp"
#include <array>
#include <algorithm>

// Pure inventory transactions: callers debit native gold only after a useful purchase succeeds.
class BuyInventory {
public:
    std::array<bool,WeaponSlots::Count> owned{};
    void Reset(const GameplaySettings& settings,int* ammo,int* reserve,const int* magazines,const int* reserves) {
        for (int i=0;i<WeaponSlots::Count;++i) {
            owned[i]=settings.startAllWeapons || WeaponSlots::Melee(i) || i==WeaponSlots::C4;
            ammo[i]=owned[i] ? (i==WeaponSlots::C4 ? settings.startBombs : magazines[i]) : 0;
            reserve[i]=owned[i] ? reserves[i] : 0;
        }
    }
    int Next(int current,int steps) const {
        int result=current;
        for (int n=0;n<std::abs(steps);++n) {
            for (int search=0;search<WeaponSlots::Count;++search) {
                result=(result+(steps<0 ? -1 : 1)+WeaponSlots::Count)%WeaponSlots::Count;
                if (owned[result]) break;
            }
        }
        return result;
    }
    enum class Result { Bought, AlreadyOwned, NotOwned, Full, NoGold, Invalid };
    Result Buy(int slot,bool ammunition,const GameplaySettings& settings,int& gold,int& ammo,int& reserve,int magazine,int reserveCapacity) {
        if (slot<0 || slot>=WeaponSlots::Count) return Result::Invalid;
        bool bomb=slot==WeaponSlots::C4;
        if (ammunition && (WeaponSlots::Melee(slot) || !owned[slot])) return Result::NotOwned;
        if (!ammunition && owned[slot] && !bomb) return Result::AlreadyOwned;
        if (bomb && ammo>=settings.maxBombs) return Result::Full;
        if (ammunition && !bomb && reserve>=reserveCapacity) return Result::Full;
        int cost=ammunition ? settings.ammoPrice[slot] : settings.weaponPrice[slot];
        if (gold<cost) return Result::NoGold;
        // A bomb is one consumable; firearm ammo fills reserves, then R transfers it into a magazine.
        gold-=cost;owned[slot]=true;
        if (bomb) ++ammo;
        else if (ammunition) reserve=std::min(reserveCapacity,reserve+settings.ammoPack[slot]);
        else { ammo=magazine;reserve=0; }
        return Result::Bought;
    }
};
