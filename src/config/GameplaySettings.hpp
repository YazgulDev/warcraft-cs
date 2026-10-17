#pragma once
#include "../combat/WeaponSlots.hpp"
#include "LoggingSettings.hpp"
#include "RenderSettings.hpp"
#include "HealthBarSettings.hpp"
#include "SpellSettings.hpp"
#include "ThirdPersonSettings.hpp"
#include "../input/ShortcutBindings.hpp"

#include "../movement/MovementSettings.hpp"
#include <array>
#include <string>

// Editable combat defaults and live settings are replaced as one complete snapshot on F8.
struct GameplaySettings {
    bool warcraftOnly=false; // Installation profile excludes CS weapons, assets, economy and samples.
    LoggingSettings logging;
    RenderSettings render;
    HealthBarSettings healthBars;
    ShortcutBindings shortcuts;
    SpellSettings spells;
    ThirdPersonSettings thirdPerson;
    bool unitViewEnabled=true;
    bool unitViewHideHoverCircles=true; // F9 borrows/restores Warcraft's native preselection presentation.
    bool unitViewPreventEnemyAttacks=true; // F9 alone can opt out of hostile basic-attack targeting.

    MovementSettings movement;
    float runeAmmoPercent = 40;
    float runePickupRadius = 160;
    bool runeAmmoAllWeapons = true;
    // Native attack damage applies to any controlled creature, including non-hero units.
    bool heroDamage = true;
    bool scaleFireRateFromAttackSpeed=false;
    // Weapon contacts use the controlled unit's native attack effects; F8 may opt out.
    bool enableAttackEffects=true;
    // Nonphysical invulnerable effect units cannot shield enemies; F8 can restore legacy ray selection.
    bool ignorePassableInvulnerableObjects=true;
    bool waterBlocksAttacks=false; // FPS and TPS share solid-surface traces; F8 can restore liquid occlusion.
    // Ordinary AWP damage is the default; instant finishing remains an explicit player opt-in.
    bool awpOneShot = false;
    // One percentage covers owned/allied units and buildings across firearms, melee and C4.
    float friendlyFirePercent = 50;
    // Q temporarily permits direct allied contacts/markers; F8 restores this saved startup preference.
    bool targetAllies=false;
    // Limit world labels to nearby FPS activity without altering native resource production.
    float floatingTextDistance=1200;
    // Startup/F8 preference; the Speed shortcut may change visibility for the current session.
    bool showSpeed=false;
    // M changes this session preference; F8 restores the editable startup choice and size.
    bool showMinimap=true;
    bool showSpellIcons=true,showSpellCooldowns=true;
    bool showUnitHud=true,showInventory=true;
    bool inventoryControls=true; // Tab/I interactions can be disabled independently of the inventory display.
    float inventoryDropDistance=80;
    bool inventoryNumbersDefault=true;
    float unitHudScalePercent=100;
    float spellIconSizePercent=7;
    float minimapSizePercent=22;
    // Temporary FPS visibility preference; ordinary map fog is restored outside FPS.
    bool disableFogOfWar=false;
    // Scale only the CS mixer; 100 preserves legacy levels and zero mutes every CS voice.
    float csVolumePercent=100;
    // Recruitment and spacing are configurable without rewriting map data or unit ownership.
    float squadRadius=600,squadFollowDistance=180,squadLeash=900;
    bool squadRearFormation=true,squadYield=true;
    bool squadPreserveCombatOrders=true; // Fighting outranks formation movement; F8 can restore legacy yielding.
    // Native costs, cooldowns and cast orders govern automatic spells; O never enables this combat AI.
    bool squadAutomaticSpells=true;
    // Right-click orders are independent of recruitment and optionally replace armed FPS secondary fire.
    bool squadReticleAttack=true,squadReticleAttackInFPS=false;
    float squadReticleDistance=3000;
    float squadSpellRadius=700,squadAreaSpellRadius=250,squadHealBelowPercent=85,squadBuffRefreshSeconds=20;
    unsigned squadSpellIntervalMs=1000;
    float squadClearance=100,squadYieldAhead=350;
    unsigned squadOrderIntervalMs=150;
    int squadMaxUnits=24;
    // Gold economy and loadout are per-map; reloading settings cannot mint another starting kit.
    bool startAllWeapons=false;
    // Default starts without C4; a positive configured count opts into starting bombs.
    int startBombs=0,maxBombs=100;
    // Paid C4 purchases may exceed the free-refill/rune amount; players can restore a purchase cap.
    bool unlimitedBombPurchases=true;
    enum class BuyAccess { Anywhere, FriendlyBuildings, Shops };
    BuyAccess buyAccess=BuyAccess::Anywhere;
    float buyRadius=600;
    std::string shopTypes="";
    // The refill shortcut is always free; ordinary purchases still debit native gold.
    std::array<int,WeaponSlots::Count> weaponPrice={1250,1550,250,2376,0,3000,500};
    // Reserve excludes the loaded magazine; buying, free refills and runes share this limit.
    std::array<int,WeaponSlots::Count> maxReserve={90,90,100,30,0,0,0};
    // C4 quick replenishment buys the same single charge as its catalog entry.
    std::array<int,WeaponSlots::Count> ammoPrice={80,60,25,125,0,3000,0};
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
