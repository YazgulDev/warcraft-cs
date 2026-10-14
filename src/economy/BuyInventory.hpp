#pragma once
#include "../config/GameplaySettings.hpp"
#include <array>
#include <algorithm>
#include <limits>

// Pure inventory transactions: callers debit native gold only after a useful purchase succeeds.
class BuyInventory {
public:
    std::array<bool,WeaponSlots::Count> owned{};
    void Reset(const GameplaySettings& settings,int* ammo,int* reserve,const int* magazines,const int* reserves) {
        for (int i=0;i<WeaponSlots::Count;++i) {
            // C4 enters the starting kit only through an explicit bomb count or the all-weapons option.
            owned[i]=settings.startAllWeapons || WeaponSlots::Melee(i) || i==WeaponSlots::Pistol ||
                (i==WeaponSlots::C4 && settings.startBombs>0);
            ammo[i]=owned[i] ? (i==WeaponSlots::C4 ? settings.startBombs : magazines[i]) : 0;
            reserve[i]=owned[i] ? reserves[i] : 0;
        }
    }
    // Free refill grants/refills C4; all-weapons also unlocks other weapons. Neither changes gold or planted timers.
    void Refill(const GameplaySettings& settings,int* ammo,int* reserve,const int* magazines,const int* reserves,bool grantAll=false) {
        for (int i=0;i<WeaponSlots::Count;++i) {
            if (grantAll || i==WeaponSlots::C4) owned[i]=true;
            if (!owned[i]) continue;
            // Refilling must never discard paid bombs above the configured free-refill amount.
            ammo[i]=i==WeaponSlots::C4 ? std::max(ammo[i],settings.maxBombs) : magazines[i];reserve[i]=reserves[i];
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
        // The optional gameplay cap does not limit ordinary repeat purchases; guard storage overflow separately.
        if (bomb && ((!settings.unlimitedBombPurchases && ammo>=settings.maxBombs) ||
            ammo==std::numeric_limits<int>::max())) return Result::Full;
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
