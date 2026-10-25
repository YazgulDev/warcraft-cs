#pragma once
#include "HitDetectionSettings.hpp"
#include "CombatVisualSettings.hpp"
#include "ItemHoverSettings.hpp"
#include "../combat/WeaponSlots.hpp"
#include "LoggingSettings.hpp"
#include "RenderSettings.hpp"
#include "ViewmodelSettings.hpp"
#include "GrenadeSettings.hpp"
#include "RpgSettings.hpp"
#include "EconomySettings.hpp"
#include "WeaponDropSettings.hpp"

#include "HalfLifeSettings.hpp"
#include "HealthBarSettings.hpp"
#include "SpellSettings.hpp"
#include "ArmyAISettings.hpp"
#include "../combat/WeaponDefaults.hpp"
#include "ThirdPersonSettings.hpp"
#include "../input/ShortcutBindings.hpp"

#include "../movement/MovementSettings.hpp"
#include <array>
#include <algorithm>
#include <string>

// Editable combat defaults and live settings are replaced as one complete snapshot on F8.
struct GameplaySettings {
    bool warcraftOnly=false; // Installation profile excludes CS weapons, assets, economy and samples.
    LoggingSettings logging;
    RenderSettings render;
    ViewmodelSettings viewmodel;
    GrenadeSettings grenades;
    RpgSettings rpg;
    EconomySettings economy;
    WeaponDropSettings weaponDrops;

    HalfLifeSettings halfLife;
    bool weaponSelectionMenu=true; // F6 category preview; false restores immediate legacy shortcuts/wheel.
    float weaponSelectionTimeout=5;
    HealthBarSettings healthBars;
    ShortcutBindings shortcuts;
    SpellSettings spells;
    ThirdPersonSettings thirdPerson;
    ArmyAISettings armyAI;
    bool shopInteraction=true;
    float shopInteractionRadius=450;
    int shotgunPellets=8;
    float shotgunSpreadDegrees=5;
    bool shotgunShellReload=true,shotgunInterruptReload=true;
    int glockBurstRounds=3;
    float glockBurstInterval=.1f,glockSecondaryCooldown=.5f;
    // Zero restores individual gun ranges; the default also covers the visible camera volume.
    float minimumFirearmRange=30000;
    std::array<Weapon,WeaponSlots::Count> weapons=WeaponDefaults::Weapons;
    std::array<float,WeaponSlots::Count> weaponMoveSpeed={221,230,250,210,250,250,220,250,250,250,245,220,230,240,250,250,250,250,250,250,250};
    std::array<float,WeaponSlots::Count> weaponModelScale={.72f,.78f,.94f,.73f,1,.95f,.85f,.9f,.94f,.8f,.8f,.75f,.8f,.8f,.9f,.9f,.9f,.9f,1,1,1};
    std::array<float,WeaponSlots::Count> weaponKick={0,0,0,0,0,0,0,2.5f,.8f,.8f,.8f,.8f,5,3,0,0,0,2,0,2,0};
    bool unitViewEnabled=true;
    bool unitViewHideHoverCircles=true; // F9 borrows/restores Warcraft's native preselection presentation.
    bool unitViewPreventEnemyAttacks=true; // F9 alone can opt out of hostile basic-attack targeting.

    MovementSettings movement;
    HitDetectionSettings hitDetection;
    CombatVisualSettings combatVisuals;
    ItemHoverSettings itemHover;
    float runeAmmoPercent = 40;
    float runePickupRadius = 160;
    bool runeAmmoAllWeapons = true;
    // Native attack damage applies to any controlled creature, including non-hero units.
    bool heroDamage = true;
    // An unavailable host adapter is distinct from a real unit with zero attack damage.
    bool unavailableAttackUsesWeaponDamage=true;
    bool scaleFireRateFromAttackSpeed=false;
    // Weapon contacts use the controlled unit's native attack effects; F8 may opt out.
    bool enableAttackEffects=true;
    bool nativeAttackSound=false; // FPS contacts keep passives while replacing the hero's weapon sound.
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
    // Currency wallets and loadout are per-map; reloading settings cannot mint another starting kit.
    bool startAllWeapons=false;
    // Default starts without C4; a positive configured count opts into starting bombs.
    int startBombs=0,maxBombs=100;
    // Paid C4 purchases may exceed the free-refill/rune amount; players can restore a purchase cap.
    bool unlimitedBombPurchases=true;
    enum class BuyAccess { Anywhere, FriendlyBuildings, Shops };
    BuyAccess buyAccess=BuyAccess::Anywhere;
    float buyRadius=600;
    std::string shopTypes="";
    // The refill shortcut is always free; purchases use the selected dollars/gold currency.
    std::array<int,WeaponSlots::Count> weaponPrice={1250,1550,250,2376,0,3000,500,650,400,1500,2350,5750,1700,3000,300,200,300,4000,200,5000,6000};
    // Reserve excludes the loaded magazine; buying, free refills and runes share this limit.
    std::array<int,WeaponSlots::Count> maxReserve={90,90,100,30,0,0,0,35,120,120,100,200,32,32,0,0,0,5,0,0,0};
    // C4 quick replenishment buys the same single charge as its catalog entry.
    std::array<int,WeaponSlots::Count> ammoPrice={80,60,25,125,0,3000,0,40,20,20,50,60,65,65,300,200,300,500,0,300,300};
    std::array<int,WeaponSlots::Count> ammoPack={30,30,12,10,0,1,0,7,20,30,50,100,8,7,1,1,1,1,0,20,20};
    bool csSky=false;
    // Every FPS map gets Warcraft's own backdrop unless the player explicitly disables it.
    bool nativeSky=true;
    std::string defaultSky="Des";
    std::array<std::string,256> tilesetSky{};
    // HE uses a fixed 300 peak independently of the controlled creature's native attack.
    std::array<float, WeaponSlots::Count> damage = {36,33,34,115,40,2500,120,54,25,26,21,32,104,80,300,0,0,100,10,100,70};
    std::array<float, WeaponSlots::Count> heroMultiplier = {1,1,1,3,1,1,3,1.6f,.75f,.8f,.65f,1,3,2.4f,0,0,0,0,1,0,0};
    float knifeSecondaryDamage = 65, swordSecondaryDamage = 240;
    static GameplaySettings Load(const std::string& filename);
    // Config alone cannot grant optional content; the private resource seal is checked on startup/F8.
    bool WeaponEnabled(int slot) const {return slot>=0 && slot<WeaponSlots::Count &&
        (!WeaponSlots::Grenade(slot) || grenades.enabled) && (slot!=WeaponSlots::RPG || (rpg.enabled && rpg.available && !warcraftOnly)) &&
        (slot<WeaponSlots::Crowbar || (!warcraftOnly && halfLife.enabled[slot-WeaponSlots::Crowbar] && halfLife.available[slot-WeaponSlots::Crowbar]));}

    float FirearmRange(int slot) const {
        return minimumFirearmRange>0 ? std::max({weapons[slot].range,minimumFirearmRange,render.drawDistance*2}) : weapons[slot].range;
    }
    float ContactDamage(int slot, bool secondary, float heroAttack) const;
    bool FinishingAWP(int slot) const { return slot == 3 && !heroDamage && awpOneShot; }
};
