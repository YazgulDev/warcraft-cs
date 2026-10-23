#pragma once
#include "../config/GameplaySettings.hpp"
#include <array>
#include <algorithm>
#include <limits>

// Pure inventory transactions debit the selected currency only after a useful purchase succeeds.
class BuyInventory {
public:
    std::array<bool,WeaponSlots::Count> owned{};
    void Reset(const GameplaySettings& settings,int* ammo,int* reserve,const int* magazines,const int* reserves) {
        for (int i=0;i<WeaponSlots::Count;++i) {
            // C4 enters the starting kit only through an explicit bomb count or the all-weapons option.
            owned[i]=settings.startAllWeapons || i==WeaponSlots::Knife || i==WeaponSlots::Sword || i==WeaponSlots::Pistol ||
                (i==WeaponSlots::C4 && settings.startBombs>0);
            if(!settings.WeaponEnabled(i))owned[i]=false;
            ammo[i]=owned[i] ? (i==WeaponSlots::C4 ? settings.startBombs : magazines[i]) : 0;
            reserve[i]=owned[i] ? reserves[i] : 0;
        }
    }
    // Free refill grants/refills C4; all-weapons also unlocks other weapons. Neither changes gold or planted timers.
    void Refill(const GameplaySettings& settings,int* ammo,int* reserve,const int* magazines,const int* reserves,bool grantAll=false) {
        for (int i=0;i<WeaponSlots::Count;++i) {
            // Missing optional resources cannot be bypassed by all-weapons, refill or retained ownership.
            if(!settings.WeaponEnabled(i))continue;
            if (grantAll || i==WeaponSlots::C4) owned[i]=true;
            if (!owned[i]) continue;
            // Refilling must never discard paid bombs above the configured free-refill amount.
            ammo[i]=i==WeaponSlots::C4 ? std::max(ammo[i],settings.maxBombs) : magazines[i];reserve[i]=reserves[i];
        }
    }
    bool Pickup(int slot,const GameplaySettings& settings,int& ammo,int& reserve) {
        if(!settings.WeaponEnabled(slot) || !WeaponSlots::Firearm(slot))return false;
        // A new drop grants one loaded gun; duplicate guns become one ammo pack without consuming a full pickup.
        if(!owned[slot]) {owned[slot]=true;ammo=settings.weapons[slot].magazine;reserve=0;return true;}
        int missing=settings.maxReserve[slot]-reserve;
        if(missing<=0)return false;
        reserve+=std::min(missing,settings.ammoPack[slot]);return true;
    }
    int Next(int current,int steps,const GameplaySettings* settings=nullptr) const {
        int result=current;
        for (int n=0;n<std::abs(steps);++n) {
            for (int search=0;search<WeaponSlots::Count;++search) {
                result=(result+(steps<0 ? -1 : 1)+WeaponSlots::Count)%WeaponSlots::Count;
                if (owned[result] && (!settings || settings->WeaponEnabled(result))) break;
            }
        }
        return result;
    }
    enum class Result { Bought, AlreadyOwned, NotOwned, Full, NoGold, Invalid };
    Result Buy(int slot,bool ammunition,const GameplaySettings& settings,int& gold,int& ammo,int& reserve,int magazine,int reserveCapacity) {
        if (!settings.WeaponEnabled(slot)) return Result::Invalid;
        bool bomb=slot==WeaponSlots::C4;
        if (ammunition && (WeaponSlots::Melee(slot) || !owned[slot])) return Result::NotOwned;
        bool grenade=WeaponSlots::Grenade(slot);
        if(grenade && !settings.grenades.enabled)return Result::Invalid;
        if (!ammunition && owned[slot] && !bomb && !grenade) return Result::AlreadyOwned;
        // The optional gameplay cap does not limit ordinary repeat purchases; guard storage overflow separately.
        if (bomb && ((!settings.unlimitedBombPurchases && ammo>=settings.maxBombs) ||
            ammo==std::numeric_limits<int>::max())) return Result::Full;
        if(grenade && ammo>=magazine)return Result::Full;
        bool energy=WeaponSlots::Energy(slot); // Cells replenish usable energy directly, without R/reload.
        if(ammunition && energy && ammo>=magazine)return Result::Full;
        if (ammunition && !bomb && !grenade && !energy && reserve>=reserveCapacity) return Result::Full;
        int cost=ammunition ? settings.ammoPrice[slot] : settings.weaponPrice[slot];
        if (gold<cost) return Result::NoGold;
        // A bomb is one consumable; firearm ammo fills reserves, then R transfers it into a magazine.
        gold-=cost;owned[slot]=true;
        if (bomb || grenade) ++ammo;
        else if (ammunition && energy)ammo=std::min(magazine,ammo+settings.ammoPack[slot]);
        else if (ammunition) reserve=std::min(reserveCapacity,reserve+settings.ammoPack[slot]);
        else { ammo=magazine;reserve=0; }
        return Result::Bought;
    }
};
