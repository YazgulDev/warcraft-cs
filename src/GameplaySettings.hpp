#pragma once
#include "WeaponSlots.hpp"
#include <array>
#include <string>

// Defaults preserve existing combat; live settings are replaced as one complete snapshot on F8.
struct GameplaySettings {
    float runeAmmoPercent = 20;
    float runePickupRadius = 160;
    bool runeAmmoAllWeapons = true;
    bool heroDamage = false;
    bool awpOneShot = true;
    // One percentage covers owned/allied units and buildings across firearms, melee and C4.
    float friendlyFirePercent = 50;
    // Recruitment and spacing are configurable without rewriting map data or unit ownership.
    float squadRadius=600,squadFollowDistance=180,squadLeash=900;
    int squadMaxUnits=24;
    std::array<float, WeaponSlots::Count> damage = {36,33,34,115,40,2500,120};
    std::array<float, WeaponSlots::Count> heroMultiplier = {1,1,1,3,1,1,3};
    float knifeSecondaryDamage = 65, swordSecondaryDamage = 240;
    static GameplaySettings Load(const std::string& filename);
    float ContactDamage(int slot, bool secondary, float heroAttack) const;
    bool FinishingAWP(int slot) const { return slot == 3 && !heroDamage && awpOneShot; }
};
