#pragma once
#include "../combat/WeaponSlots.hpp"
#include "../movement/MovementSettings.hpp"
#include <array>
#include <string>

// Defaults preserve existing combat; live settings are replaced as one complete snapshot on F8.
struct GameplaySettings {
    MovementSettings movement;
    float runeAmmoPercent = 20;
    float runePickupRadius = 160;
    bool runeAmmoAllWeapons = true;
    bool heroDamage = false;
    bool awpOneShot = true;
    // One percentage covers owned/allied units and buildings across firearms, melee and C4.
    float friendlyFirePercent = 50;
    // Limit world labels to nearby FPS activity without altering native resource production.
    float floatingTextDistance=1200;
    // Scale only the CS mixer; 100 preserves legacy levels and zero mutes every CS voice.
    float csVolumePercent=100;
    // Recruitment and spacing are configurable without rewriting map data or unit ownership.
    float squadRadius=600,squadFollowDistance=180,squadLeash=900;
    int squadMaxUnits=24;
    // Gold economy and loadout are per-map; reloading settings cannot mint another starting kit.
    bool startAllWeapons=false;
    int startBombs=20,maxBombs=100;
    enum class BuyAccess { Anywhere, FriendlyBuildings, Shops };
    BuyAccess buyAccess=BuyAccess::Anywhere;
    float buyRadius=600;
    std::string shopTypes="";
    // F7 is always a free refill; ordinary purchases still debit native gold.
    std::array<int,WeaponSlots::Count> weaponPrice={625,775,125,1188,0,50,250};
    std::array<int,WeaponSlots::Count> ammoPrice={80,60,25,125,0,200,0};
    std::array<int,WeaponSlots::Count> ammoPack={30,30,12,10,0,1,0};
    bool csSky=false;
    // Every FPS map gets Warcraft's own backdrop unless the player explicitly disables it.
    bool nativeSky=true;
    std::string defaultSky="Des";
    std::array<std::string,256> tilesetSky{};
    std::array<float, WeaponSlots::Count> damage = {36,33,34,115,40,2500,120};
    std::array<float, WeaponSlots::Count> heroMultiplier = {1,1,1,3,1,1,3};
    float knifeSecondaryDamage = 65, swordSecondaryDamage = 240;
    static GameplaySettings Load(const std::string& filename);
    float ContactDamage(int slot, bool secondary, float heroAttack) const;
    bool FinishingAWP(int slot) const { return slot == 3 && !heroDamage && awpOneShot; }
};
