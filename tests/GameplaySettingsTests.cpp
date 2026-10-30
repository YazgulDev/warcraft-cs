#include "../src/config/GameplaySettings.hpp"
#include "MovementStrafeInput.hpp"
#include "../src/inventory/AmmoRecovery.hpp"
#include "../src/combat/CombatDamage.hpp"
#include "../src/combat/UnitScaling.hpp"
#include <cassert>
#include <filesystem>
#include <cmath>
#include <fstream>
#include <vector>
#include <tuple>
#include <windows.h>
namespace wc3 { void Log(const char*,...) {} }
int main() {
    // Headless regressions report assertion details to the captured journal instead of a desktop dialog.
    _set_error_mode(_OUT_TO_STDERR);
    GameplaySettings settings;
    // Appended CS guns remain available without optional Half-Life assets and own complete nonzero tuning.
    for(int slot=WeaponSlots::TMP;slot<=WeaponSlots::G3SG1;++slot) {
        assert(settings.WeaponEnabled(slot) && WeaponSlots::Firearm(slot) && !WeaponSlots::HalfLife(slot));
        assert(settings.weapons[slot].magazine>0 && settings.maxReserve[slot]>0 && settings.weaponMoveSpeed[slot]>=50);
        assert(settings.heroMultiplier[slot]>0 && settings.damage[slot]>0 && settings.weaponPrice[slot]>0 && settings.ammoPack[slot]>0);
    }
    assert(WeaponSlots::Scoped(WeaponSlots::G3SG1) && !WeaponSlots::Scoped(WeaponSlots::FAMAS));
    assert(settings.grenades.smokeVisibilityDistance==96 && settings.grenades.smokeBlocksAI && settings.grenades.smokeOpacity==.99f);
    // Missing configs and F8 share the requested defaults; invalid values are rejected at the INI boundary.
    assert(settings.economy.currency==EconomySettings::Currency::Dollars && settings.economy.startingMoney==800);
    assert(settings.economy.killReward==200 && settings.economy.maximumMoney==16000);
    assert(settings.weaponDrops.enabled && settings.weaponDrops.chancePercent==.5f);
    assert(settings.rpg.radius==600 && settings.damage[WeaponSlots::RPG]==1800);
    assert(settings.damage[WeaponSlots::HE]==600 && settings.heroMultiplier[WeaponSlots::M3]==6 && settings.heroMultiplier[WeaponSlots::XM1014]==4.8f);
    assert(settings.ContactDamage(WeaponSlots::M3,false,100)==600 && std::abs(settings.ContactDamage(WeaponSlots::XM1014,false,100)-480)<.001f);
    assert(settings.ContactDamage(WeaponSlots::RPG,false,100)==1800 && settings.ContactDamage(WeaponSlots::HE,false,100)==600);
    auto economyPath=(std::filesystem::current_path()/"economy-reload-test.ini").string();
    {std::ofstream f(economyPath);f<<"[Economy]\nCurrency=gold\nStartingMoney=99999\nMaximumMoney=2000\nKillReward=nan\n[WeaponDrops]\nEnabled=false\nChancePercent=999\nLifetimeSeconds=-2\nPickupRadius=nan\nMaximumDrops=0\n[RPG]\nRadius=nan\nDamage=nan\n";}
    auto economyReload=GameplaySettings::Load(economyPath);
    assert(economyReload.economy.currency==EconomySettings::Currency::Gold && economyReload.economy.startingMoney==2000);
    assert(economyReload.economy.killReward==200 && !economyReload.weaponDrops.enabled);
    assert(economyReload.weaponDrops.chancePercent==100 && economyReload.weaponDrops.lifetimeSeconds==1);
    assert(economyReload.weaponDrops.pickupRadius==160 && economyReload.weaponDrops.maximumDrops==1);
    assert(economyReload.rpg.radius==600 && economyReload.damage[WeaponSlots::RPG]==1800);
    WritePrivateProfileStringA("Economy","Currency","broken",economyPath.c_str());
    WritePrivateProfileStringA("WeaponDrops","Enabled","broken",economyPath.c_str());
    WritePrivateProfileStringA("WeaponDrops","ChancePercent","nan",economyPath.c_str());
    economyReload=GameplaySettings::Load(economyPath);
    assert(economyReload.economy.currency==EconomySettings::Currency::Dollars && economyReload.weaponDrops.enabled && economyReload.weaponDrops.chancePercent==.5f);
    // Direct F8 reload upgrades the old stock blast once; a versioned custom radius remains editable.
    WritePrivateProfileStringA("RPG","Radius","375",economyPath.c_str());
    assert(GameplaySettings::Load(economyPath).rpg.radius==600);
    WritePrivateProfileStringA("RPG","BlastVersion","2",economyPath.c_str());
    assert(GameplaySettings::Load(economyPath).rpg.radius==375);
    DeleteFileA(economyPath.c_str());

    // A missing host adapter permits fixed configured damage; an actual zero-attack creature does not.
    assert(settings.ContactDamage(0,false,-1)==settings.damage[0]);
    assert(settings.ContactDamage(0,false,0)==0);
    settings.unavailableAttackUsesWeaponDamage=false;assert(settings.ContactDamage(0,false,-1)==0);
    settings.unavailableAttackUsesWeaponDamage=true;
    // F8 reads a fresh validated snapshot; invalid fallback policy does not turn missing statistics into zero damage.
    char fallbackDirectory[MAX_PATH]={};GetCurrentDirectoryA(MAX_PATH,fallbackDirectory);
    std::string fallbackPath=std::string(fallbackDirectory)+"\\warcraft-host-damage-fallback.ini";
    for(const char* policy:{"zero","weapon","broken",""}) {
        WritePrivateProfileStringA("Damage","UnavailableAttackFallback",policy,fallbackPath.c_str());
        auto reloaded=GameplaySettings::Load(fallbackPath);
        assert(reloaded.unavailableAttackUsesWeaponDamage==(std::string(policy)!="zero"));
    }
    DeleteFileA(fallbackPath.c_str());
    // Every firearm reaches visible distant targets, even when an older INI retains a short individual range.
    for(int slot=0;slot<WeaponSlots::Count;++slot)if(WeaponSlots::Firearm(slot))assert(settings.FirearmRange(slot)==30000);
    assert(settings.movement.useNativeUnitCollision);
    // Exercise the actual INI reload boundary; malformed values retain the enabled movement fix.
    char collisionFolder[MAX_PATH]={};GetCurrentDirectoryA(MAX_PATH,collisionFolder);
    const std::string collisionReload=std::string(collisionFolder)+"\\native-unit-collision-test.ini";
    for(auto value:{"true","false","broken","","nan"}) {
        WritePrivateProfileStringA("Movement","UseNativeUnitCollision",value,collisionReload.c_str());
        auto parsed=GameplaySettings::Load(collisionReload);
        assert(parsed.movement.useNativeUnitCollision==(strcmp(value,"false")!=0));
    }
    DeleteFileA(collisionReload.c_str());
    assert(settings.ContactDamage(0,false,100)==100);
    assert(settings.ContactDamage(4,true,100)==162.5f);
    // Default AWP hits use their configured base damage rather than health-dependent finishing damage.
    assert(!settings.FinishingAWP(3));
    assert(CombatDamage::Amount(settings.ContactDamage(3,false,100),false,
        CombatDamage::FinishesTarget(settings.FinishingAWP(3),false),5000)==300);
    assert(settings.friendlyFirePercent==50);
    assert(settings.floatingTextDistance==1200);
    assert(!settings.showSpeed);
    assert(!settings.disableFogOfWar);
    assert(!settings.healthBars.allies && !settings.healthBars.enemies && settings.healthBars.distance==2000);
    assert(settings.csVolumePercent==100);
    assert(settings.render.maxFPS==144 && settings.render.drawDistance==5000);
    assert(!settings.startAllWeapons && settings.startBombs==0 && settings.maxBombs==100 && settings.unlimitedBombPurchases);
    assert(settings.buyAccess==GameplaySettings::BuyAccess::Anywhere && !settings.csSky);
    assert(settings.weaponPrice[0]==1250 && settings.weaponPrice[1]==1550 && settings.weaponPrice[2]==250);
    // C4 catalog and quick replenishment charge the same requested default price.
    assert(settings.weaponPrice[3]==2376 && settings.weaponPrice[5]==3000 && settings.weaponPrice[6]==500);
    assert(settings.ammoPrice[5]==3000 && settings.enableAttackEffects);
    assert(settings.squadRadius==600 && settings.squadMaxUnits==24 && settings.squadFollowDistance==180 && settings.squadLeash==900);
    settings.heroDamage=true;settings.heroMultiplier[0]=2;
    assert(settings.ContactDamage(0,false,28)==56);
    assert(settings.ContactDamage(4,true,40)==65);
    assert(settings.ContactDamage(6,true,40)==240);
    assert(!settings.FinishingAWP(3));
    assert(CombatDamage::Amount(settings.ContactDamage(0,false,28),true,false,100)==28);
    settings.friendlyFirePercent=25;
    assert(CombatDamage::Amount(settings.ContactDamage(0,false,28),true,false,100,settings.friendlyFirePercent)==14);
    AmmoRecovery recovery;int magazine=0,reserve=0;
    assert(recovery.Restore(0,20,30,90,magazine,reserve)==24 && magazine==6 && reserve==18);
    magazine=29;reserve=89;assert(recovery.Restore(0,20,30,90,magazine,reserve)==2);
    assert(magazine==30 && reserve==90);
    // A held E cannot mint integer ammunition faster than the configured fractional amount.
    int bomb=0,none=0;
    for (int i=0;i<4;++i) { recovery.Restore(5,20,1,0,bomb,none);assert(bomb==0); }
    recovery.Restore(5,20,1,0,bomb,none);assert(bomb==1);
    recovery.Reset();bomb=0;recovery.Restore(5,0,1,0,bomb,none);assert(bomb==0);
    // Native IAS scales cadence only when enabled; malformed stats retain the base interval.
    assert(UnitScaling::FireInterval(.1f,false,2)==.1f);
    assert(UnitScaling::FireInterval(.1f,true,2)==.05f);
    assert(UnitScaling::FireInterval(.1f,true,NAN)==.1f);
    assert(UnitScaling::FireInterval(.1f,true,0)==.1f);
    assert(std::abs(UnitScaling::FireInterval(.1f,true,100)-.02f)<.0001f);
    assert(UnitScaling::MoveScale(false,450,300,1.2f)==1.2f);
    assert(UnitScaling::MoveScale(true,450,300,1.2f)==1.5f);
    assert(UnitScaling::MoveScale(true,0,300,1)==0);
    assert(UnitScaling::MoveScale(true,NAN,300,.5f)==.5f);
    magazine=reserve=0;recovery.Reset();
    assert(recovery.Restore(0,40,30,50,magazine,reserve)==32 && magazine==12 && reserve==20);
    char directory[MAX_PATH];GetCurrentDirectoryA(MAX_PATH,directory);
    std::string path=std::string(directory)+"\\settings-test.ini";
    // Validate configurable reserve capacity and both optional native-stat modes at the real INI boundary.
    {std::ofstream file(path);file<<"[AK47]\nMaxReserve=250\n[Combat]\nScaleFireRateFromAttackSpeed=true\n[Movement]\nScaleFromUnitMoveSpeed=true\nUnitMoveSpeedReference=250\n";}
    auto scaling=GameplaySettings::Load(path);
    assert(scaling.maxReserve[0]==250 && scaling.scaleFireRateFromAttackSpeed);
    assert(scaling.movement.scaleFromUnitMoveSpeed && scaling.movement.unitMoveSpeedReference==250);
    for(const auto& test:{std::pair<const char*,int>{"",90},{"bad",90},{"nan",90},{"-1",0},{"0",0},{"999999",100000}}) {
        WritePrivateProfileStringA("AK47","MaxReserve",test.first,path.c_str());
        assert(GameplaySettings::Load(path).maxReserve[0]==test.second);
    }
    DeleteFileA(path.c_str());
    // Real INI loading covers explicit legacy mode, bounded targets, bad input and live reload.
    const struct {const char* fps;const char* distance;int expectedFPS;float expectedDistance;} renderCases[]={
        {"","",144,5000},{"0","5000",0,5000},{"240","15000",240,15000},
        {"1","1",30,3000},{"999","99999",360,20000},{"-2","-3",0,3000},
        {"nan","inf",144,5000},{"broken","wrong",144,5000},{"120.5","12000.5",120,12000.5f}
    };
    int renderIndex=0;
    for (const auto& test:renderCases) {
        auto sample=path+".render-"+std::to_string(renderIndex++);
        {std::ofstream file(sample);file<<"[Rendering]\nMaxFPS="<<test.fps<<"\nDrawDistance="<<test.distance<<"\n";}
        auto loaded=GameplaySettings::Load(sample);
        assert(loaded.render.maxFPS==test.expectedFPS && loaded.render.drawDistance==test.expectedDistance);
        DeleteFileA(sample.c_str());
    }
    auto renderReload=path+".render-reload";
    WritePrivateProfileStringA("Rendering","MaxFPS","72",renderReload.c_str());
    WritePrivateProfileStringA("Rendering","DrawDistance","7000",renderReload.c_str());
    assert(GameplaySettings::Load(renderReload).render.maxFPS==72);
    WritePrivateProfileStringA("Rendering","MaxFPS","144",renderReload.c_str());
    WritePrivateProfileStringA("Rendering","DrawDistance","15000",renderReload.c_str());
    auto reloaded=GameplaySettings::Load(renderReload);
    assert(reloaded.render.maxFPS==144 && reloaded.render.drawDistance==15000);
    DeleteFileA(renderReload.c_str());
    // The clean F9 default is independently reloadable without changing other views' target-marker settings.
    for(auto value:{"true","false","1","0","broken",""}) {
        WritePrivateProfileStringA("UnitView","HideHoverCircles",value,renderReload.c_str());
        assert(GameplaySettings::Load(renderReload).unitViewHideHoverCircles==(strcmp(value,"false") && strcmp(value,"0")));
    }
    DeleteFileA(renderReload.c_str());
    // Exercise the real optional HUD/fog preference boundary and live snapshot reloads.
    const struct { const char* value; bool enabled; } speedCases[] = {
        {"",false},{"false",false},{"true",true},{"TrUe",true},{"0",false},{"1",true},{"broken",false}
    };
    int speedIndex=0;
    for (const auto& test:speedCases) {
        const auto sample=path+".speed-"+std::to_string(speedIndex++);
        { std::ofstream file(sample);file<<"[Interface]\nShowSpeed="<<test.value<<"\nDisableFogOfWar="<<test.value<<
            "\nShowAllyHealthBars="<<test.value<<"\nShowEnemyHealthBars="<<test.value<<"\n"; }
        assert(GameplaySettings::Load(sample).showSpeed==test.enabled);
        assert(GameplaySettings::Load(sample).disableFogOfWar==test.enabled);
        assert(GameplaySettings::Load(sample).healthBars.allies==test.enabled && GameplaySettings::Load(sample).healthBars.enemies==test.enabled);
        DeleteFileA(sample.c_str());
    }
    const auto speedReload=path+".speed-reload";
    DeleteFileA(speedReload.c_str());assert(!GameplaySettings::Load(speedReload).showSpeed);
    WritePrivateProfileStringA("Interface","ShowSpeed","true",speedReload.c_str());
    WritePrivateProfileStringA("Interface","DisableFogOfWar","true",speedReload.c_str());
    assert(GameplaySettings::Load(speedReload).showSpeed);
    assert(GameplaySettings::Load(speedReload).disableFogOfWar);
    WritePrivateProfileStringA("Interface","ShowSpeed","false",speedReload.c_str());
    WritePrivateProfileStringA("Interface","DisableFogOfWar","false",speedReload.c_str());
    assert(!GameplaySettings::Load(speedReload).showSpeed);
    assert(!GameplaySettings::Load(speedReload).disableFogOfWar);DeleteFileA(speedReload.c_str());
    // Reload independent team switches and distance through the actual Windows INI boundary.
    const auto barsReload=path+".bars-reload";
    WritePrivateProfileStringA("Interface","ShowAllyHealthBars","true",barsReload.c_str());
    WritePrivateProfileStringA("Interface","ShowEnemyHealthBars","false",barsReload.c_str());
    auto bars=GameplaySettings::Load(barsReload);assert(bars.healthBars.allies && !bars.healthBars.enemies);
    WritePrivateProfileStringA("Interface","ShowAllyHealthBars","false",barsReload.c_str());
    WritePrivateProfileStringA("Interface","ShowEnemyHealthBars","true",barsReload.c_str());
    bars=GameplaySettings::Load(barsReload);assert(!bars.healthBars.allies && bars.healthBars.enemies);
    for(const auto& test: {std::pair<const char*,float>{"",2000},{"bad",2000},{"nan",2000},{"-1",0},{"0",0},{"850",850},{"20000",15000}}) {
        WritePrivateProfileStringA("Interface","HealthBarDistance",test.first,barsReload.c_str());
        assert(GameplaySettings::Load(barsReload).healthBars.distance==test.second);
    }
    DeleteFileA(barsReload.c_str());
    // Exercise the actual INI boundary, including absent/invalid defaults and explicit opt-in/out reloads.
    const struct { const char* value; bool enabled; } awpCases[] = {
        {"",false},{"false",false},{"true",true},{"TrUe",true},{"0",false},{"1",true},{"broken",false}
    };
    int awpIndex=0;
    for (const auto& test:awpCases) {
        std::string sample=path+".awp-"+std::to_string(awpIndex++);
        { std::ofstream file(sample);file<<"[Damage]\nMode=weapon\nAWPOneShot="<<test.value<<"\n"; }
        auto configured=GameplaySettings::Load(sample);DeleteFileA(sample.c_str());
        assert(configured.awpOneShot==test.enabled && configured.FinishingAWP(3)==test.enabled);
        assert(!configured.FinishingAWP(0));
        float damage=CombatDamage::Amount(configured.ContactDamage(3,false,0),false,
            CombatDamage::FinishesTarget(configured.FinishingAWP(3),false),5000);
        assert(test.enabled ? damage>5000 : damage==115);
        assert(CombatDamage::Amount(configured.ContactDamage(3,false,0),true,
            CombatDamage::FinishesTarget(configured.FinishingAWP(3),true),5000)==57.5f);
        configured.heroDamage=true;assert(!configured.FinishingAWP(3));
    }
    std::string awpReload=path+".awp-reload";DeleteFileA(awpReload.c_str());
    assert(!GameplaySettings::Load(awpReload).FinishingAWP(3));
    WritePrivateProfileStringA("Damage","Mode","weapon",awpReload.c_str());
    WritePrivateProfileStringA("Damage","AWPOneShot","true",awpReload.c_str());
    assert(GameplaySettings::Load(awpReload).FinishingAWP(3));
    WritePrivateProfileStringA("Damage","AWPOneShot","false",awpReload.c_str());
    assert(!GameplaySettings::Load(awpReload).FinishingAWP(3));DeleteFileA(awpReload.c_str());
    // Invalid formation values cannot produce zero slots or a combat leash shorter than the spacing.
    { std::ofstream file(path);file<<"[Runes]\nAmmoPercent=35\nAmmoWeapons=current\nPickupRadius=999\n[Damage]\nMode=hero\n[AK47]\nHeroMultiplier=2.5\nDamage=nan\n[USP]\nDamage=broken\n[Squad]\nRecruitRadius=9999\nMaxUnits=0\nFollowDistance=0\nCombatLeash=0\n"; }
    settings=GameplaySettings::Load(path);DeleteFileA(path.c_str());
    assert(settings.runeAmmoPercent==35 && !settings.runeAmmoAllWeapons && settings.runePickupRadius==400);
    assert(settings.heroDamage && settings.heroMultiplier[0]==2.5f);
    assert(settings.damage[0]==36 && settings.damage[2]==34);
    assert(settings.squadRadius==2000 && settings.squadMaxUnits==1 && settings.squadFollowDistance==80 && settings.squadLeash==280);
    settings=GameplaySettings::Load(path);assert(settings.runeAmmoPercent==40 && settings.runeAmmoAllWeapons && settings.heroDamage);
    assert(settings.friendlyFirePercent==50); // Missing keys keep legacy 50% damage.
    assert(settings.csVolumePercent==100); // Missing Audio keys retain the previous CS mix.
    // Exercise real INI parsing, sanitization and repeated snapshot loads rather than only assigning the field.
    struct PercentCase { const char* value;float expected; };
    const PercentCase cases[]={{"25",25},{"0",0},{"100",100},{"12.5",12.5f},
        {"-10",0},{"200",100},{"nan",50},{"broken",50},{"",50}};
    int index=0;
    for (const auto& test : cases) {
        const auto sample=path+std::to_string(++index);
        { std::ofstream file(sample);file<<"[Damage]\nFriendlyFirePercent="<<test.value<<"\n"; }
        settings=GameplaySettings::Load(sample);DeleteFileA(sample.c_str());
        assert(settings.friendlyFirePercent==test.expected);
    }
    // Exercise the actual INI loader for mute, fractional gains, bounds and malformed audio settings.
    const PercentCase volumes[]={{"0",0},{"50",50},{"100",100},{"12.5",12.5f},
        {"-10",0},{"200",100},{"nan",100},{"inf",100},{"broken",100},{"",100}};
    for (const auto& test : volumes) {
        const auto sample=path+std::to_string(++index);
        { std::ofstream file(sample);file<<"[Audio]\nCSVolumePercent="<<test.value<<"\n"; }
        settings=GameplaySettings::Load(sample);DeleteFileA(sample.c_str());
        assert(settings.csVolumePercent==test.expected);
    }
    // Reloading an existing INI must replace the audio snapshot rather than retaining the startup gain.
    WritePrivateProfileStringA("Audio","CSVolumePercent","35",path.c_str());
    assert(GameplaySettings::Load(path).csVolumePercent==35);
    WritePrivateProfileStringA("Audio","CSVolumePercent","0",path.c_str());
    assert(GameplaySettings::Load(path).csVolumePercent==0);
    WritePrivateProfileStringA("Audio","CSVolumePercent","100",path.c_str());
    assert(GameplaySettings::Load(path).csVolumePercent==100);
    // Real INI snapshots support swaps/reload and reject a layout as a unit, without duplicate defaults.
    {std::ofstream file(path);file<<"[Shortcuts]\nSpell1=Q\nSpell2=E\nPickup=Z\nSpeed=TAB\n[Spells]\nEnabled=false\nAimDistance=850\nSlot1Ability=AHtb\nSlot1Order=852095\nSlot1Target=unit\n";}
    auto customized=GameplaySettings::Load(path);
    assert(customized.shortcuts.Key(ShortcutAction::Spell1)=='Q' && customized.shortcuts.Key(ShortcutAction::Spell2)=='E');
    assert(customized.shortcuts.Key(ShortcutAction::Pickup)=='Z' && customized.shortcuts.Key(ShortcutAction::Speed)==VK_TAB);
    assert(!customized.spells.enabled && customized.spells.aimDistance==850 && customized.spells.ability[0]=="AHtb" && customized.spells.order[0]==852095 && customized.spells.target[0]=="unit");
    WritePrivateProfileStringA("Shortcuts","Spell1","E",path.c_str());customized=GameplaySettings::Load(path);
    assert(customized.shortcuts.Key(ShortcutAction::Spell1)=='Z' && customized.shortcuts.Key(ShortcutAction::Speed)=='P');
    WritePrivateProfileStringA("Spells","Slot1Ability","invalid",path.c_str());
    WritePrivateProfileStringA("Spells","Slot1Order","851983",path.c_str());
    WritePrivateProfileStringA("Spells","Slot1Target","invalid",path.c_str());
    WritePrivateProfileStringA("Spells","AimDistance","NaN",path.c_str());customized=GameplaySettings::Load(path);
    assert(customized.spells.ability[0].empty() && !customized.spells.order[0] && customized.spells.target[0]=="auto" && customized.spells.aimDistance==3000);
    // Out-of-range and fractional IDs must never turn into another engine command through clamping/rounding.
    for(const char* invalid:{"999999","-1","852095.01","NaN","852095x",""}) {
        WritePrivateProfileStringA("Spells","Slot1Order",invalid,path.c_str());assert(!GameplaySettings::Load(path).spells.order[0]);
    }
    DeleteFileA(path.c_str());
    // Diagnostic defaults, bounds and F8-style reloads use the actual INI parser.
    { std::ofstream file(path); file << "[Logging]\nDetailed=false\nIntervalMs=1\nMaxFileMB=999\nArchiveCount=-1\n"; }
    settings=GameplaySettings::Load(path);
    assert(!settings.logging.detailed && settings.logging.graphicsDebug && settings.logging.intervalMs==100 && settings.logging.maxFileMB==64 && settings.logging.archiveCount==0);
    // F8 snapshots validate both observer opt-out and malformed/missing fallback independently of TRACE.
    for(const char* value : {"false","FALSE","0"}) {
        WritePrivateProfileStringA("Logging","GraphicsDebug",value,path.c_str());
        assert(!GameplaySettings::Load(path).logging.graphicsDebug);
    }
    for(const char* value : {"true","TRUE","1","","bad","nan"}) {
        WritePrivateProfileStringA("Logging","GraphicsDebug",value,path.c_str());
        assert(GameplaySettings::Load(path).logging.graphicsDebug);
    }
    WritePrivateProfileStringA("Logging","Detailed","true",path.c_str());
    WritePrivateProfileStringA("Logging","IntervalMs","2500",path.c_str());
    settings=GameplaySettings::Load(path); assert(settings.logging.detailed && settings.logging.intervalMs==2500);
    { std::ofstream file(path); file << "[Logging]\nDetailed=invalid\nIntervalMs=nan\nMaxFileMB=broken\nArchiveCount=inf\n"; }
    settings=GameplaySettings::Load(path);
    assert(settings.logging.detailed && settings.logging.graphicsDebug && settings.logging.intervalMs==1000 && settings.logging.maxFileMB==8 && settings.logging.archiveCount==3);
    DeleteFileA(path.c_str());
    // Visibility settings reject malformed values and bound work to the world camera's range.
    // Combined display/spell controls share a conflict-checked layout and survive a fresh INI load.
    {std::ofstream file(path);file<<"[Interface]\nShowMinimap=true\nShowAllyHealthBars=true\nShowEnemyHealthBars=true\n[Shortcuts]\nMinimap=Y\n";}
    auto combined=GameplaySettings::Load(path);
    assert(combined.showMinimap && combined.healthBars.allies && combined.healthBars.enemies);
    assert(combined.shortcuts.Key(ShortcutAction::Minimap)=='Y' && combined.shortcuts.Key(ShortcutAction::HealthBars)=='K');
    assert(combined.shortcuts.Key(ShortcutAction::Spell4)=='V' && combined.shortcuts.Key(ShortcutAction::Speed)=='P');
    WritePrivateProfileStringA("Shortcuts","Minimap","V",path.c_str());
    combined=GameplaySettings::Load(path); // A minimap/spell collision must restore the entire safe default layout.
    assert(combined.shortcuts.Key(ShortcutAction::Minimap)=='M' && combined.shortcuts.Key(ShortcutAction::Spell4)=='V');
    DeleteFileA(path.c_str());
    // Minimap preferences use the real INI parser and remain safe across repeated F8 snapshots.
    const PercentCase sizes[]={{"10",10},{"25.5",25.5f},{"-5",10},{"200",40},{"nan",22},{"inf",22},{"",22},{"bad",22}};
    for (const auto& test : sizes) {
        {std::ofstream file(path);file<<"[Interface]\nMinimapSizePercent="<<test.value<<"\n";}
        assert(GameplaySettings::Load(path).minimapSizePercent==test.expected);
    }
    for (const char* value : {"true","1","TRUE","broken","","nan"}) {
        WritePrivateProfileStringA("Interface","ShowMinimap",value,path.c_str());
        assert(GameplaySettings::Load(path).showMinimap);
    }
    for (const char* value : {"false","0","FALSE"}) {
        WritePrivateProfileStringA("Interface","ShowMinimap",value,path.c_str());
        assert(!GameplaySettings::Load(path).showMinimap);
    }
    DeleteFileA(path.c_str());assert(GameplaySettings::Load(path).showMinimap);
    // Spell HUD preferences are re-read by F8 and bound before layout allocates textures.
    for(const char* option:{"ShowSpellIcons","ShowSpellCooldowns"}) {
        for(const char* value:{"false","0","FALSE"}) {
            WritePrivateProfileStringA("Interface",option,value,path.c_str());
            auto hud=GameplaySettings::Load(path);
            assert(option[9]=='I' ? !hud.showSpellIcons : !hud.showSpellCooldowns);
        }
        for(const char* value:{"true","1","TRUE","broken","","nan"}) {
            WritePrivateProfileStringA("Interface",option,value,path.c_str());
            auto hud=GameplaySettings::Load(path);
            assert(option[9]=='I' ? hud.showSpellIcons : hud.showSpellCooldowns);
        }
    }
    const PercentCase spellSizes[]={{"4",4},{"7.5",7.5f},{"-5",4},{"200",14},{"nan",7},{"inf",7},{"",7},{"bad",7}};
    for(const auto& test:spellSizes) {
        WritePrivateProfileStringA("Interface","SpellIconSizePercent",test.value,path.c_str());
        assert(GameplaySettings::Load(path).spellIconSizePercent==test.expected);
    }
    DeleteFileA(path.c_str());
    auto defaultHud=GameplaySettings::Load(path);
    assert(defaultHud.showSpellIcons && defaultHud.showSpellCooldowns && defaultHud.spellIconSizePercent==7);
    const PercentCase distances[]={{"0",0},{"900",900},{"-10",0},{"99999",5000},{"nan",1200},{"broken",1200}};
    for (const auto& test : distances) {
        const auto sample=path+std::to_string(++index);
        { std::ofstream file(sample);file<<"[Interface]\nFloatingTextDistance="<<test.value<<"\n"; }
        settings=GameplaySettings::Load(sample);DeleteFileA(sample.c_str());
        assert(settings.floatingTextDistance==test.expected);
    }
    // Invalid prices/counts cannot produce credits or huge allocations; unsafe sky paths use the fallback.
    { std::ofstream file(path);file<<"[Loadout]\nMode=all\nBombCount=5000\nMaxBombs=30\n[Buy]\nAccess=shops\nRadius=99999\nAllowFreeRefill=true\n[AK47]\nPrice=-50\nAmmoPrice=nan\nAmmoPack=0\n[Sky]\nEnabled=false\nDefault=../../secret\nW=blue\n"; }
    settings=GameplaySettings::Load(path);DeleteFileA(path.c_str());
    assert(settings.startAllWeapons && settings.startBombs==30 && settings.maxBombs==30);
    assert(settings.buyAccess==GameplaySettings::BuyAccess::Shops && settings.buyRadius==3000);
    assert(settings.weaponPrice[0]==0 && settings.ammoPrice[0]==80 && settings.ammoPack[0]==1);
    assert(!settings.csSky && settings.defaultSky=="Des" && settings.tilesetSky['W']=="blue");
    { std::ofstream file(path);file<<"[Buy]\nAccess=friendly\n[Loadout]\nBombCount=0\n[Sky]\nA=forest\nWarcraftEnabled=true\n"; }
    settings=GameplaySettings::Load(path);DeleteFileA(path.c_str());
    assert(settings.buyAccess==GameplaySettings::BuyAccess::FriendlyBuildings && settings.startBombs==0 && settings.tilesetSky['A']=="forest");
    assert(settings.nativeSky && !settings.csSky);
    // Real INI parsing bounds every movement knob before the physics snapshot is applied.
    { std::ofstream file(path); file << "[Movement]\nBunnyHop=false\nAutoJump=false\nJumpBoostPercent=250\nMaxBunnySpeed=1\nAirAcceleration=nan\nJumpSpeed=0\nGravity=9999\nStepHeight=-5\n"; }
    settings=GameplaySettings::Load(path);DeleteFileA(path.c_str());
    assert(!settings.movement.bunnyHop && !settings.movement.autoJump);
    assert(settings.movement.jumpBoostPercent==100 && settings.movement.maxBunnySpeed==250);
    assert(settings.movement.airAcceleration==10 && settings.movement.jumpSpeed==1);
    assert(settings.movement.gravity==3000 && settings.movement.stepHeight==0);
    { std::ofstream file(path); file << "[Movement]\nBunnyHop=broken\nAutoJump=\nJumpBoostPercent=inf\nMaxBunnySpeed=nan\nJumpSpeed=broken\nGravity=nan\nStepHeight=200\n"; }
    settings=GameplaySettings::Load(path);DeleteFileA(path.c_str());
    assert(settings.movement.bunnyHop && settings.movement.autoJump);
    assert(settings.movement.jumpBoostPercent==0 && settings.movement.maxBunnySpeed==1000);
    assert(settings.movement.jumpSpeed==268.328f && settings.movement.gravity==800 && settings.movement.stepHeight==64);
    WritePrivateProfileStringA("Movement","JumpBoostPercent","15",path.c_str());
    assert(GameplaySettings::Load(path).movement.jumpBoostPercent==15);
    WritePrivateProfileStringA("Movement","JumpBoostPercent","0",path.c_str());
    assert(GameplaySettings::Load(path).movement.jumpBoostPercent==0);
    // Consume the actual parsed snapshot in physics, then replace it as F8 does while airborne.
    { std::ofstream file(path); file << "[Movement]\nMaxBunnySpeed=400\nJumpBoostPercent=0\nAirAcceleration=10\n"; }
    MovementPhysics movement; movement.Configure(GameplaySettings::Load(path).movement); movement.Reset(0);
    MoveInput run; run.forward=1;
    for(int frame=0;frame<60;++frame) movement.Step(run,1.f/60,250);
    for(int frame=0;frame<360;++frame) {
        auto input=MovementStrafeInput(movement,float(frame)/60);
        movement.Step(input,1.f/60,250); movement.ResolveFloor(0);
    }
    assert(movement.Speed()>590 && movement.Speed()<=600.01f);
    WritePrivateProfileStringA("Movement","MaxBunnySpeed","300",path.c_str());
    float feet=movement.FeetZ(), vertical=movement.VerticalVelocity();
    movement.Configure(GameplaySettings::Load(path).movement);
    assert(movement.FeetZ()==feet && movement.VerticalVelocity()==vertical);
    movement.Step(MovementStrafeInput(movement,6),1.f/60,250);
    assert(movement.Speed()<=450.01f);
    DeleteFileA(path.c_str());
    // Geometry settings use the same real config/F8 path and retain current motion on reload.
    for(const char* value:{"", "broken", "nan", "inf"}) {
        WritePrivateProfileStringA("Movement","MaxWalkSlope",value,path.c_str());
        WritePrivateProfileStringA("Movement","HullRadius",value,path.c_str());
        auto collision=GameplaySettings::Load(path).movement;
        assert(collision.maxWalkSlope==46 && collision.hullRadius==24);
    }
    WritePrivateProfileStringA("Movement","MaxWalkSlope","999",path.c_str());
    WritePrivateProfileStringA("Movement","HullRadius","-10",path.c_str());
    auto hull=GameplaySettings::Load(path).movement;assert(hull.maxWalkSlope==75 && hull.hullRadius==8);
    WritePrivateProfileStringA("Movement","MaxWalkSlope","-10",path.c_str());
    WritePrivateProfileStringA("Movement","HullRadius","999",path.c_str());
    hull=GameplaySettings::Load(path).movement;assert(hull.maxWalkSlope==15 && hull.hullRadius==64);
    WritePrivateProfileStringA("Movement","MaxWalkSlope","50",path.c_str());
    WritePrivateProfileStringA("Movement","HullRadius","18",path.c_str());
    feet=movement.FeetZ();vertical=movement.VerticalVelocity();movement.Configure(GameplaySettings::Load(path).movement);
    assert(movement.MaxWalkSlope()==50 && movement.HullRadius()==18 && movement.FeetZ()==feet && movement.VerticalVelocity()==vertical);
    DeleteFileA(path.c_str());
    // New squad parameters use the real INI boundary; live snapshots clamp invalid distances/refresh rates.
    // Mesh detail reloads through the existing snapshot without resetting airborne motion.
    for(const char* value:{"", "broken", "true", "false", "TRUE"}) {
        WritePrivateProfileStringA("Movement","ObjectMeshCollision",value,path.c_str());
        movement.Configure(GameplaySettings::Load(path).movement);
        assert(movement.ObjectMeshCollision()==(std::strcmp(value,"false")!=0));
        assert(movement.FeetZ()==feet && movement.VerticalVelocity()==vertical);
    }
    DeleteFileA(path.c_str());
    assert(ShortcutBindings{}.Key(ShortcutAction::SquadOrders)=='Y');
    // Recovery settings use the same validated F8 snapshot and retain the active motor state.
    assert(ShortcutBindings{}.Key(ShortcutAction::RecoverPosition)==VK_HOME);
    // The selected mode is a real config/F8 switch; defaults and unknown names select classic.
    assert(MovementSettings{}.classicCollision && !MovementSettings{}.platformRecovery);
    auto missingMode=GameplaySettings::Load(path).movement;
    assert(missingMode.classicCollision && !missingMode.platformRecovery);
    for(const char* value:{"", "broken", "classic", "CLASSIC", "hull", "HULL", "recovery", "RECOVERY"}) {
        WritePrivateProfileStringA("Movement","Mode",value,path.c_str());
        movement.Configure(GameplaySettings::Load(path).movement);
        assert(movement.PlatformRecovery()==(!_stricmp(value,"recovery")));
        assert(movement.ClassicCollision()==(_stricmp(value,"recovery") && _stricmp(value,"hull")));
        assert(movement.FeetZ()==feet && movement.VerticalVelocity()==vertical);
    }
    WritePrivateProfileStringA("Movement","Mode","classic",path.c_str());
    assert(ShortcutBindings::Parse("HOME")==VK_HOME && ShortcutBindings::Label(VK_HOME)=="HOME");
    for(const char* value:{"", "broken", "nan", "inf"}) {
        WritePrivateProfileStringA("Movement","RecoveryDistance",value,path.c_str());
        WritePrivateProfileStringA("Movement","RecoverInvalidPosition",value,path.c_str());
        auto recover=GameplaySettings::Load(path).movement;
        assert(recover.recoverInvalidPosition && recover.recoveryDistance==512);
    }
    WritePrivateProfileStringA("Movement","RecoveryDistance","9999",path.c_str());
    WritePrivateProfileStringA("Movement","RecoverInvalidPosition","false",path.c_str());
    assert(GameplaySettings::Load(path).movement.recoveryDistance==2048);
    WritePrivateProfileStringA("Movement","RecoveryDistance","-1",path.c_str());
    auto recover=GameplaySettings::Load(path);movement.Configure(recover.movement);
    assert(!movement.RecoverInvalidPosition() && movement.RecoveryDistance()==64);
    assert(movement.FeetZ()==feet && movement.VerticalVelocity()==vertical);
    WritePrivateProfileStringA("Shortcuts","RecoverPosition","NONE",path.c_str());
    assert(GameplaySettings::Load(path).shortcuts.Key(ShortcutAction::RecoverPosition)==0);
    DeleteFileA(path.c_str());
    assert(ShortcutBindings::Parse("SEMICOLON")==VK_OEM_1 && ShortcutBindings::Parse(";")==VK_OEM_1 && ShortcutBindings::Label(VK_OEM_1)==";");
    {std::ofstream file(path);file<<"[Shortcuts]\nSpeed=SEMICOLON\n";}
    auto oldSemicolon=GameplaySettings::Load(path);assert(oldSemicolon.shortcuts.Key(ShortcutAction::Speed)==VK_OEM_1 && oldSemicolon.shortcuts.Key(ShortcutAction::SquadOrders)=='Y');
    // Upgrade the old default at startup/F8 too; an existing custom Y action wins without resetting the layout.
    for(auto oldKey:{"SEMICOLON",";"}) {
        {std::ofstream file(path);file<<"[Shortcuts]\nSquadOrders="<<oldKey<<"\n";}
        assert(GameplaySettings::Load(path).shortcuts.Key(ShortcutAction::SquadOrders)=='Y');
        WritePrivateProfileStringA("Shortcuts","Speed","Y",path.c_str());
        auto collision=GameplaySettings::Load(path);
        assert(collision.shortcuts.Key(ShortcutAction::Speed)=='Y' && collision.shortcuts.Key(ShortcutAction::SquadOrders)==0);
    }
    WritePrivateProfileStringA("Shortcuts","SquadOrders",nullptr,path.c_str());
    assert(GameplaySettings::Load(path).shortcuts.Key(ShortcutAction::SquadOrders)==0);
    WritePrivateProfileStringA("Shortcuts","SquadOrders","G",path.c_str());assert(GameplaySettings::Load(path).shortcuts.Key(ShortcutAction::SquadOrders)=='G');
    WritePrivateProfileStringA("Shortcuts","SquadOrders","NONE",path.c_str());assert(GameplaySettings::Load(path).shortcuts.Key(ShortcutAction::SquadOrders)==0);
    for(auto value:{"true","1","false","0","broken",""}) {
        {std::ofstream file(path);file<<"[Squad]\nReticleAttack="<<value<<"\nReticleAttackInFPS="<<value<<"\nPreserveCombatOrders="<<value<<"\nReticleAttackDistance=nan\n";}
        auto focus=GameplaySettings::Load(path);
        assert(focus.squadReticleAttack==(strcmp(value,"false") && strcmp(value,"0")));
        assert(focus.squadPreserveCombatOrders==(strcmp(value,"false") && strcmp(value,"0")));
        assert(focus.squadReticleAttackInFPS==(!strcmp(value,"true") || !strcmp(value,"1")) && focus.squadReticleDistance==3000);
    }
    for(auto value:{"0","99999","800"}) {
        WritePrivateProfileStringA("Squad","ReticleAttackDistance",value,path.c_str());
        assert(GameplaySettings::Load(path).squadReticleDistance==(!strcmp(value,"0") ? 100 : !strcmp(value,"99999") ? 15000 : 800));
    }
    {std::ofstream file(path);file<<"[Squad]\nRearFormation=false\nYieldToPlayer=0\nPlayerClearance=9999\nYieldAheadDistance=2\nOrderIntervalMs=0\n";}
    settings=GameplaySettings::Load(path);
    assert(!settings.squadRearFormation && !settings.squadYield && settings.squadClearance==300 && settings.squadYieldAhead==300 && settings.squadOrderIntervalMs==50);
    {std::ofstream file(path);file<<"[Squad]\nRearFormation=broken\nYieldToPlayer=nan\nPlayerClearance=nan\nYieldAheadDistance=inf\nOrderIntervalMs=bad\n";}
    settings=GameplaySettings::Load(path);
    assert(settings.squadRearFormation && settings.squadYield && settings.squadClearance==100 && settings.squadYieldAhead==350 && settings.squadOrderIntervalMs==150);
    WritePrivateProfileStringA("Squad","PlayerClearance","160",path.c_str());
    assert(GameplaySettings::Load(path).squadClearance==160);DeleteFileA(path.c_str());
    // The real parser supports an F8 on/off transition, with safe defaults for malformed input.
    for(auto value : {"true","1","false","0","broken","","nan"}) {
        {std::ofstream file(path);file<<"[Combat]\nEnableAttackEffects="<<value<<"\n";}
        auto effects=GameplaySettings::Load(path);
        assert(effects.enableAttackEffects==(strcmp(value,"false")!=0 && strcmp(value,"0")!=0));
    }
    WritePrivateProfileStringA("Combat","EnableAttackEffects","false",path.c_str());
    assert(!GameplaySettings::Load(path).enableAttackEffects);
    WritePrivateProfileStringA("Combat","EnableAttackEffects","true",path.c_str());
    assert(GameplaySettings::Load(path).enableAttackEffects);DeleteFileA(path.c_str());
    // The object filter follows the same validated on/off and live reload contract as native passives.
    for(auto value : {"true","1","false","0","broken","","nan"}) {
        {std::ofstream file(path);file<<"[Combat]\nIgnorePassableInvulnerableObjects="<<value<<"\n";}
        assert(GameplaySettings::Load(path).ignorePassableInvulnerableObjects==(strcmp(value,"false")!=0 && strcmp(value,"0")!=0));
    }
    WritePrivateProfileStringA("Combat","IgnorePassableInvulnerableObjects","false",path.c_str());
    assert(!GameplaySettings::Load(path).ignorePassableInvulnerableObjects);
    WritePrivateProfileStringA("Combat","IgnorePassableInvulnerableObjects","true",path.c_str());
    assert(GameplaySettings::Load(path).ignorePassableInvulnerableObjects);DeleteFileA(path.c_str());
    // Real INI reload can restore the purchase cap; malformed values keep unlimited purchases enabled.
    for(auto value : {"true","1","false","0","broken","","nan"}) {
        {std::ofstream file(path);file<<"[Loadout]\nUnlimitedBombPurchases="<<value<<"\n";}
        assert(GameplaySettings::Load(path).unlimitedBombPurchases==(strcmp(value,"false")!=0 && strcmp(value,"0")!=0));
    }
    WritePrivateProfileStringA("Loadout","UnlimitedBombPurchases","false",path.c_str());
    assert(!GameplaySettings::Load(path).unlimitedBombPurchases);
    WritePrivateProfileStringA("Loadout","UnlimitedBombPurchases","true",path.c_str());
    assert(GameplaySettings::Load(path).unlimitedBombPurchases);DeleteFileA(path.c_str());
    // The new display preferences use the real F8 parser with bounded scale and safe fallbacks.
    for(auto value : {"false","0","true","1","broken","nan",""}) {
        {std::ofstream file(path);file<<"[Interface]\nShowUnitHud="<<value<<"\nShowInventory="<<value<<"\nUnitHudScalePercent="<<value<<"\n";}
        settings=GameplaySettings::Load(path);
        bool enabled=strcmp(value,"false") && strcmp(value,"0");
        assert(settings.showUnitHud==enabled && settings.showInventory==enabled);
        assert(settings.unitHudScalePercent>=60 && settings.unitHudScalePercent<=160);
    }
    for(auto scale : {"-20","999","nan","95"}) {
        WritePrivateProfileStringA("Interface","UnitHudScalePercent",scale,path.c_str());
        float expected=!strcmp(scale,"-20") ? 60.f : !strcmp(scale,"999") ? 160.f : !strcmp(scale,"nan") ? 100.f : 95.f;
        assert(GameplaySettings::Load(path).unitHudScalePercent==expected);
    }
    DeleteFileA(path.c_str());
    // New inventory defaults cannot steal an older customized Tab/I shortcut during parser-only upgrades.
    {std::ofstream file(path);file<<"[Shortcuts]\nSpeed=TAB\nPickup=I\n[Interface]\nInventoryControls=false\n[Inventory]\nDropDistance=999\n";}
    settings=GameplaySettings::Load(path);
    assert(!settings.inventoryControls && settings.inventoryDropDistance==128);
    // The new default loads without migration and explicit weapon-first choices survive F8.
    assert(settings.inventoryNumbersDefault);
    for(auto value:{"false","0","true","1","broken"}) {
        WritePrivateProfileStringA("Inventory","NumbersByDefault",value,path.c_str());
        assert(GameplaySettings::Load(path).inventoryNumbersDefault==(strcmp(value,"false") && strcmp(value,"0")));
    }
    assert(settings.shortcuts.Key(ShortcutAction::Speed)==VK_TAB && settings.shortcuts.Key(ShortcutAction::Pickup)=='I');
    assert(settings.shortcuts.Key(ShortcutAction::InventoryNumbers)==0 && settings.shortcuts.Key(ShortcutAction::InventoryPanel)==0);
    for(auto value : {"-1","nan","100"}) {
        WritePrivateProfileStringA("Inventory","DropDistance",value,path.c_str());
        float expected=!strcmp(value,"-1") ? 16.f : !strcmp(value,"nan") ? 80.f : 100.f;
        assert(GameplaySettings::Load(path).inventoryDropDistance==expected);
    }
    // New options use the real loader, including malformed/range handling and a saved U action on upgrade.
    {std::ofstream f(path);f<<"[ArmyAI]\nWorkers=99\nIntervalMs=nan\nProductionRetrySeconds=99999\nEnabled=false\nWorker=hpea\nHall=bad\n[Shops]\nRadius=-1\n[Shotguns]\nPellets=0\nSpreadDegrees=99\n[Deagle]\nMagazine=9999\nFireInterval=0\nReloadSeconds=nan\nPrice=777\n[Shortcuts]\nPickup=U\n";}
    for(auto value:{"true","1","false","0","","bad","nan"}) {
        WritePrivateProfileStringA("ArmyAI","FollowActor",value,path.c_str());
        assert(GameplaySettings::Load(path).armyAI.followActor==(!strcmp(value,"true") || !strcmp(value,"1")));
    }
    WritePrivateProfileStringA("ArmyAI","FollowActor",nullptr,path.c_str());
    assert(!GameplaySettings::Load(path).armyAI.followActor);
    auto added=GameplaySettings::Load(path);
    assert(!added.armyAI.enabled && added.armyAI.workers==30 && added.armyAI.intervalMs==1500);
    assert(added.armyAI.productionRetrySeconds==3600);
    assert(added.armyAI.worker=="hpea" && added.armyAI.hall.empty() && added.shopInteractionRadius==100);
    assert(added.shotgunPellets==1 && added.shotgunSpreadDegrees==15);
    assert(added.weapons[WeaponSlots::Deagle].magazine==1000 && added.weapons[WeaponSlots::Deagle].interval==.025f);
    assert(added.weapons[WeaponSlots::Deagle].reload==2.2f && added.weaponPrice[WeaponSlots::Deagle]==777);
    assert(added.shortcuts.Key(ShortcutAction::Pickup)=='U' && added.shortcuts.Key(ShortcutAction::ArmyAI)==0);
    DeleteFileA(path.c_str());
    {std::ofstream f(path);f<<"[Combat]\nMinimumFirearmRange=30000\n[M3]\nRange=1200\n[Rendering]\nDrawDistance=20000\n";}
    auto reach=GameplaySettings::Load(path);
    assert(reach.weapons[WeaponSlots::M3].range==1200 && reach.FirearmRange(WeaponSlots::M3)==40000);
    for(auto value:{"0","-1","nan","inf","","bad","999999","16000"}) {
        WritePrivateProfileStringA("Combat","MinimumFirearmRange",value,path.c_str());
        reach=GameplaySettings::Load(path);
        float expected=(!strcmp(value,"0") || !strcmp(value,"-1")) ? 0 : !strcmp(value,"999999") ? 50000 : !strcmp(value,"16000") ? 16000 : 30000;
        assert(reach.minimumFirearmRange==expected);
        assert(reach.FirearmRange(WeaponSlots::M3)==(expected>0 ? std::max(expected,40000.f) : 1200));
    }
    // F8 takes the whole new snapshot; explicit long per-gun settings also survive the shared extension.
    WritePrivateProfileStringA("M3","Range","49000",path.c_str());
    assert(GameplaySettings::Load(path).FirearmRange(WeaponSlots::M3)==49000);
    WritePrivateProfileStringA("M3","Range","999999",path.c_str());
    assert(GameplaySettings::Load(path).weapons[WeaponSlots::M3].range==50000);
    // Both fresh and F8 snapshots validate the new secondary mode without changing the ordinary single-shot interval.
    DeleteFileA(path.c_str());
    for(auto value:{"-2","999","nan","bad","","0.2"}) {
        {std::ofstream f(path);f<<"[Glock]\nSecondaryBurstRounds="<<value<<"\nSecondaryBurstInterval="<<value<<"\nSecondaryCooldown="<<value<<"\n";}
        auto glock=GameplaySettings::Load(path);
        assert(glock.glockBurstRounds>=1 && glock.glockBurstRounds<=5);
        assert(glock.glockBurstInterval>=.025f && glock.glockBurstInterval<=1);
        assert(glock.glockSecondaryCooldown>=.1f && glock.glockSecondaryCooldown<=5);
        assert(glock.weapons[WeaponSlots::Glock].interval==.15f);
        if(!strcmp(value,"nan") || !strcmp(value,"bad") || !*value)
            assert(glock.glockBurstRounds==3 && glock.glockBurstInterval==.1f && glock.glockSecondaryCooldown==.5f);
    }
    WritePrivateProfileStringA("Glock","SecondaryBurstRounds","4",path.c_str());
    WritePrivateProfileStringA("Glock","SecondaryBurstInterval","0.08",path.c_str());
    auto glock=GameplaySettings::Load(path);assert(glock.glockBurstRounds==4 && glock.glockBurstInterval==.08f);
    // Fresh installs and F8 snapshots enable shell transfers/interrupts; explicit legacy preferences survive.
    for(auto value:{"true","1","false","0","bad",""}) {
        {std::ofstream f(path);f<<"[Shotguns]\nShellByShellReload="<<value<<"\nInterruptReloadOnFire="<<value<<"\n";}
        auto shells=GameplaySettings::Load(path);bool enabled=strcmp(value,"false") && strcmp(value,"0");
        assert(shells.shotgunShellReload==enabled && shells.shotgunInterruptReload==enabled);
    }
    // Startup/F8 bounds and fallbacks protect the viewmodel clocks and GL transforms.
    for(auto value:{"false","0","true","1","nan","bad","","-1","999"}) {
        {std::ofstream f(path);f<<"[Viewmodel]\nBobEnabled="<<value<<"\nMuzzleFlash="<<value<<"\nBobAmount="<<value<<"\nBobFrequency="<<value<<"\nBobResponse="<<value<<"\nMuzzleFlashMilliseconds="<<value<<"\nMuzzleFlashScale="<<value<<"\n";}
        const auto visual=GameplaySettings::Load(path).viewmodel;
        bool enabled=strcmp(value,"false") && strcmp(value,"0");assert(visual.bob==enabled && visual.muzzleFlash==enabled);
        assert(visual.bobAmount>=0 && visual.bobAmount<=3 && visual.bobFrequency>=.5f && visual.bobFrequency<=4);
        assert(visual.bobResponse>=1 && visual.bobResponse<=30 && visual.flashMilliseconds>=10 && visual.flashMilliseconds<=200);
        assert(visual.flashScale>=0 && visual.flashScale<=3);
        if(!strcmp(value,"nan") || !strcmp(value,"bad") || !*value)
            assert(visual.bobAmount==1 && visual.bobFrequency==1.8f && visual.bobResponse==12 && visual.flashMilliseconds==65 && visual.flashScale==1);
    }
    WritePrivateProfileStringA("Viewmodel","BobAmount","0.7",path.c_str());
    WritePrivateProfileStringA("Viewmodel","MuzzleFlashMilliseconds","90",path.c_str());
    assert(GameplaySettings::Load(path).viewmodel.bobAmount==.7f && GameplaySettings::Load(path).viewmodel.flashMilliseconds==90);
    // Grenade limits and effect clocks come from the same validated startup/F8 snapshot as all weapons.
    for(auto value:{"-1","99999","nan","inf","bad","","0.4"}) {
        {std::ofstream f(path);f<<"[Grenades]\nFuseSeconds="<<value<<"\nThrowSpeed="<<value<<"\nGravity="<<value<<"\nBounce="<<value<<"\nMaxActive="<<value<<"\n[HEGrenade]\nCapacity="<<value<<"\nDamage="<<value<<"\nRadius="<<value<<"\n[Flashbang]\nBlindSeconds="<<value<<"\nUnitStunSeconds="<<value<<"\n[SmokeGrenade]\nDurationSeconds="<<value<<"\nOpacity="<<value<<"\nVisibilityDistance="<<value<<"\n";}
        const auto profile=GameplaySettings::Load(path);const auto& g=profile.grenades;
        assert(g.fuse>=.2f && g.fuse<=10 && g.throwSpeed>=50 && g.throwSpeed<=2000 && g.gravity>=50 && g.gravity<=2000);
        assert(g.bounce>=0 && g.bounce<=.9f && g.maxActive>=1 && g.maxActive<=128 && g.heRadius>=32 && g.heRadius<=2000);
        assert(g.smokeVisibilityDistance>=16 && g.smokeVisibilityDistance<=600);
        assert(g.flashSeconds>=0 && g.flashSeconds<=10 && g.flashUnitStunSeconds>=0 && g.flashUnitStunSeconds<=10 && g.smokeSeconds>=1 && g.smokeSeconds<=60 && g.smokeOpacity>=0 && g.smokeOpacity<=1);
        assert(profile.weapons[WeaponSlots::HE].magazine>=1 && profile.weapons[WeaponSlots::HE].magazine<=100);
        assert(profile.ContactDamage(WeaponSlots::HE,false,999)==profile.damage[WeaponSlots::HE]);
        if(!strcmp(value,"nan") || !strcmp(value,"inf") || !strcmp(value,"bad") || !*value)assert(g.fuse==1.5f && g.throwSpeed==1125 && g.gravity==660 && g.smokeSeconds==18);
    }
    {std::ofstream f(path);f<<"[Grenades]\nEnabled=false\nFuseSeconds=2.25\n[Flashbang]\nCapacity=3\nUnitStunSeconds=0\n[SmokeGrenade]\nDurationSeconds=7\nVisibilityDistance=64\nBlocksAI=false\n";}
    const auto grenades=GameplaySettings::Load(path);assert(!grenades.grenades.enabled && grenades.grenades.fuse==2.25f && grenades.weapons[WeaponSlots::Flashbang].magazine==3 && grenades.grenades.flashUnitStunSeconds==0 && grenades.grenades.smokeSeconds==7 && grenades.grenades.smokeVisibilityDistance==64 && !grenades.grenades.smokeBlocksAI);
    // World geometry tuning uses the real F8 parser and never permits a zero/unsafe scale.
    for(auto value:{"-1","99999","nan","bad","","0.5"}) {
        WritePrivateProfileStringA("Grenades","WorldModelScale",value,path.c_str());WritePrivateProfileStringA("Grenades","WorldSpinDegrees",value,path.c_str());
        auto world=GameplaySettings::Load(path).grenades;assert(world.worldModelScale>=.25f && world.worldModelScale<=4 && world.worldSpin>=0 && world.worldSpin<=1440);
        if(!strcmp(value,"nan") || !strcmp(value,"bad") || !*value)assert(world.worldModelScale==1 && world.worldSpin==360);
    }
    // Startup/F8 menu preferences use bounded defaults, including malformed and non-finite text.
    for(const char* value:{"", "bad", "nan", "inf", "-1", "99", "2.5"}) {
        {std::ofstream file(path);file<<"[WeaponSelection]\nEnabled="<<value<<"\nTimeoutSeconds="<<value<<"\n";}
        auto menu=GameplaySettings::Load(path);
        assert(menu.weaponSelectionMenu);
        float timeout=!strcmp(value,"-1") ? .5f : !strcmp(value,"99") ? 15.f : !strcmp(value,"2.5") ? 2.5f : 5.f;
        assert(menu.weaponSelectionTimeout==timeout);
    }
    for(const char* value:{"false","0","FALSE"}) {
        {std::ofstream file(path);file<<"[WeaponSelection]\nEnabled="<<value<<"\nTimeoutSeconds=3\n";}
        auto menu=GameplaySettings::Load(path);assert(!menu.weaponSelectionMenu && menu.weaponSelectionTimeout==3);
    }
    // Old stock defaults upgrade both on startup/F8; partial customizations and versioned settings survive.
    for(const char* config:{"[Grenades]\nThrowSpeed=750\nGravity=800\n", "[Grenades]\nThrowSpeed=750\nGravity=800\nTrajectoryVersion=1\n"}) {
        {std::ofstream file(path);file<<config;}
        auto g=GameplaySettings::Load(path).grenades;assert(g.throwSpeed==1125 && g.gravity==660);
    }
    for(const char* config:{"[Grenades]\nThrowSpeed=1000\nGravity=800\n", "[Grenades]\nThrowSpeed=750\nGravity=800\nTrajectoryVersion=2\n"}) {
        {std::ofstream file(path);file<<config;}
        auto g=GameplaySettings::Load(path).grenades;assert(g.gravity==800 && g.throwSpeed==(!strstr(config,"1000") ? 750 : 1000));
    }
    for(const char* value:{"-1","9999","nan","inf","bad","","0.75"}) {
        {std::ofstream file(path);file<<"[Grenades]\nLightGravityScale="<<value<<"\n";}
        auto scale=GameplaySettings::Load(path).grenades.lightGravityScale;
        assert(scale>=.1f && scale<=2);
        if(!strcmp(value,"nan") || !strcmp(value,"inf") || !strcmp(value,"bad") || !*value)assert(std::abs(scale-10.f/11)<.00001f);
    }
    // The optional resource seal is checked by the real startup/F8 parser, with no OpenGL dependency.
    auto rpgRoot=std::filesystem::current_path()/("warcraft-rpg-test-"+std::to_string(GetCurrentProcessId()));
    assert(std::filesystem::create_directory(rpgRoot));auto assets=rpgRoot/"assets";std::filesystem::create_directory(assets);
    auto rpgPath=(rpgRoot/"WarcraftCS.ini").string();{std::ofstream f(rpgPath);f<<"[RPG]\nDamage=777\nMaximumSpeed=1234\n";}
    assert(!GameplaySettings::Load(rpgPath).rpg.available);
    const char* rpgFiles[]={"rpg.wcg","rpg.wca","buy/rpg.wcg","world/rpgrocket.wcg","sounds/hl_rpg_fire.wav","sounds/hl_rpg_motor.wav","sounds/hl_rpg_explode.wav"};
    {
        std::ofstream seal(assets/"rpg.ready",std::ios::binary);seal.write("WRP1",4);
        for(auto name:rpgFiles) {
            auto output=assets/name;std::filesystem::create_directories(output.parent_path());std::string bytes="synthetic optional asset";
            {std::ofstream f(output,std::ios::binary);f<<bytes;}
            uint64_t n=bytes.size(),hash=14695981039346656037ull;for(unsigned char c:bytes){hash^=c;hash*=1099511628211ull;}
            seal.write(reinterpret_cast<char*>(&n),8);seal.write(reinterpret_cast<char*>(&hash),8);
        }
    }
    auto optional=GameplaySettings::Load(rpgPath);assert(optional.rpg.available && optional.WeaponEnabled(WeaponSlots::RPG) && optional.damage[17]==777 && optional.rpg.maximumSpeed==1234);
    {std::ofstream f(assets/"world/rpgrocket.wcg",std::ios::app);f<<"corrupt";}
    assert(!GameplaySettings::Load(rpgPath).rpg.available);
    for(auto key:{"Radius","LaunchSpeed","MaximumSpeed","Acceleration","IgnitionSeconds","BoostSeconds","LaunchGravity","LifetimeSeconds","InheritedVelocity","WorldModelScale","TrailSeconds","TrailSize","MaxActive"}) {
        WritePrivateProfileStringA("RPG",key,"nan",rpgPath.c_str());auto g=GameplaySettings::Load(rpgPath).rpg;
        assert(std::isfinite(g.maximumSpeed) && g.radius>=32 && g.lifetime>=1 && g.maxActive>=1);
    }
    // Independent seals must be checked by the real startup/F8 loader, not just converter tests.
    const std::vector<std::vector<const char*>> guestFiles={
        {"crowbar.wcg","crowbar.wca","buy/crowbar.wcg","sounds/hl_cbar_miss1.wav","sounds/hl_cbar_hit1.wav","sounds/hl_cbar_hitbod1.wav",
        "sounds/hl_cbar_hit2.wav","sounds/hl_cbar_hitbod2.wav","sounds/hl_cbar_hitbod3.wav","sounds/hl_cbar_ground1.wav","sounds/hl_cbar_ground2.wav","sounds/hl_cbar_ground3.wav"},
        {"gauss.wcg","gauss.wca","buy/gauss.wcg","sounds/hl_gauss2.wav","sounds/hl_gauss_charge.wav"},
        {"egon.wcg","egon.wca","buy/egon.wcg","sounds/hl_egon_windup2.wav","sounds/hl_egon_run3.wav","sounds/hl_egon_off1.wav"}};
    const char* seals[]={"crowbar.ready","gauss.ready","egon.ready"};
    for(int i=0;i<3;++i) {
        assert(!GameplaySettings::Load(rpgPath).WeaponEnabled(WeaponSlots::Crowbar+i));
        std::ofstream seal(assets/seals[i],std::ios::binary);seal.write("WHL1",4);
        for(auto name:guestFiles[i]) {
            auto output=assets/name;std::filesystem::create_directories(output.parent_path());std::string bytes="synthetic guest asset";
            {std::ofstream f(output,std::ios::binary);f<<bytes;}
            uint64_t n=bytes.size(),hash=14695981039346656037ull;for(unsigned char c:bytes){hash^=c;hash*=1099511628211ull;}
            seal.write(reinterpret_cast<char*>(&n),8);seal.write(reinterpret_cast<char*>(&hash),8);
        }
        seal.close();assert(GameplaySettings::Load(rpgPath).WeaponEnabled(WeaponSlots::Crowbar+i));
    }
    {std::ofstream f(rpgPath);f<<"[TauCannon]\nChargeSeconds=2\nChargedDamage=777\nMagazine=55\nMaxReserve=999\n[HalfLife]\nBeamWidth=8\n";}
    auto hl=GameplaySettings::Load(rpgPath);assert(hl.halfLife.tauChargeSeconds==2 && hl.halfLife.tauChargedDamage==777 && hl.halfLife.beamWidth==8);
    assert(hl.damage[WeaponSlots::Tau]==100 && hl.damage[WeaponSlots::Gluon]==70 && !hl.nativeAttackSound);
    for(const char* value:{"true","false","broken"}) {
        WritePrivateProfileStringA("Combat","NativeAttackSound",value,rpgPath.c_str());hl=GameplaySettings::Load(rpgPath);
        assert(hl.nativeAttackSound==!strcmp(value,"true"));
    }
    WritePrivateProfileStringA("HalfLife","BeamEffects","false",rpgPath.c_str());
    WritePrivateProfileStringA("HalfLife","OwnedSprites","false",rpgPath.c_str());
    WritePrivateProfileStringA("HalfLife","ImpactSize","999",rpgPath.c_str());hl=GameplaySettings::Load(rpgPath);
    assert(!hl.halfLife.beamEffects && !hl.halfLife.ownedSprites && hl.halfLife.impactSize==100);
    WritePrivateProfileStringA("HalfLife","ImpactSize","nan",rpgPath.c_str());hl=GameplaySettings::Load(rpgPath);assert(hl.halfLife.impactSize==18);
    assert(hl.halfLife.tauSpinStartRate==1 && hl.halfLife.tauSpinEndRate==3 && hl.halfLife.tauPitchStart==1 && hl.halfLife.tauPitchEnd==2.5f && hl.halfLife.crowbarSurfaceSounds);
    WritePrivateProfileStringA("Crowbar","SurfaceSounds","false",rpgPath.c_str());assert(!GameplaySettings::Load(rpgPath).halfLife.crowbarSurfaceSounds);
    WritePrivateProfileStringA("TauCannon","SpinEndRate","6",rpgPath.c_str());assert(GameplaySettings::Load(rpgPath).halfLife.tauSpinEndRate==6);
    assert(hl.weapons[WeaponSlots::Tau].magazine==55 && hl.maxReserve[WeaponSlots::Tau]==0);
    assert(hl.ContactDamage(WeaponSlots::Tau,false,9999)==100 && hl.ContactDamage(WeaponSlots::Gluon,false,9999)==70);
    for(auto value:{"nan","inf","bad","","-1","999999"}) {
        WritePrivateProfileStringA("TauCannon","ChargeSeconds",value,rpgPath.c_str());
        WritePrivateProfileStringA("HalfLife","BeamWidth",value,rpgPath.c_str());hl=GameplaySettings::Load(rpgPath);
        for(auto key:{"SpinStartRate","SpinEndRate","ChargePitchStart","ChargePitchEnd"})WritePrivateProfileStringA("TauCannon",key,value,rpgPath.c_str());
        hl=GameplaySettings::Load(rpgPath);auto feedback=hl.halfLife;
        assert(std::isfinite(feedback.tauSpinStartRate) && feedback.tauSpinStartRate>=.25f && feedback.tauSpinEndRate>=feedback.tauSpinStartRate && feedback.tauSpinEndRate<=8);
        assert(std::isfinite(feedback.tauPitchStart) && feedback.tauPitchStart>=.25f && feedback.tauPitchEnd>=feedback.tauPitchStart && feedback.tauPitchEnd<=4);
        assert(std::isfinite(hl.halfLife.tauChargeSeconds) && hl.halfLife.tauChargeSeconds>=.1f && hl.halfLife.tauChargeSeconds<=10);
        assert(std::isfinite(hl.halfLife.beamWidth) && hl.halfLife.beamWidth>=.5f && hl.halfLife.beamWidth<=20);
    }
    WritePrivateProfileStringA("TauCannon","Enabled","false",rpgPath.c_str());
    assert(!GameplaySettings::Load(rpgPath).WeaponEnabled(WeaponSlots::Tau));
    {std::ofstream f(assets/"egon.wcg",std::ios::app);f<<"changed";}
    hl=GameplaySettings::Load(rpgPath);assert(!hl.WeaponEnabled(WeaponSlots::Gluon) && hl.WeaponEnabled(WeaponSlots::Crowbar));
    // This test owns the freshly created temporary directory exclusively.
    assert(rpgRoot.parent_path()==std::filesystem::current_path());std::filesystem::remove_all(rpgRoot);
    // The production startup/F8 parser preserves both wallets' preferences and bounds new visual/combat tuning.
    {std::ofstream f(path);f<<"[Economy]\nCurrency=gold\nFallbackToGold=false\n[Combat]\nPreciseUnitHits=false\nHeadshots=false\nHeadshotMultiplier=3\n[Interface]\nItemNames=false\nItemNameDistance=2200\nItemNameRadius=18\n";}
    auto aiming=GameplaySettings::Load(path);
    assert(aiming.economy.currency==EconomySettings::Currency::Gold && !aiming.economy.fallbackToGold);
    assert(!aiming.hitDetection.preciseModels && !aiming.hitDetection.headshots && aiming.hitDetection.headshotMultiplier==3);
    assert(!aiming.itemHover.enabled && aiming.itemHover.distance==2200 && aiming.itemHover.radius==18);
    for(const char* value:{"nan","inf","bad","","-1","999999"}) {
        {std::ofstream f(path);f<<"[Combat]\nHeadshotMultiplier="<<value<<"\n[Interface]\nItemNameDistance="<<value<<"\nItemNameRadius="<<value<<"\n";}
        aiming=GameplaySettings::Load(path);
        assert(aiming.hitDetection.headshotMultiplier>=1 && aiming.hitDetection.headshotMultiplier<=10);
        assert(aiming.itemHover.distance>=160 && aiming.itemHover.distance<=5000 && aiming.itemHover.radius>=4 && aiming.itemHover.radius<=128);
        assert(aiming.economy.currency==EconomySettings::Currency::Dollars && aiming.economy.fallbackToGold);
        assert(aiming.hitDetection.preciseModels && aiming.hitDetection.headshots && aiming.itemHover.enabled);
        if(!strcmp(value,"nan") || !strcmp(value,"inf") || !strcmp(value,"bad") || !*value)
            assert(aiming.hitDetection.headshotMultiplier==2 && aiming.itemHover.distance==1200 && aiming.itemHover.radius==32);
    }
    // Effects consume the same bounded F8 snapshot; disable flags, signed offsets and custom lifetimes survive loading.
    // Camera response is a validated live-reload preference, independent of physical movement tuning.
    {std::ofstream f(path);f<<"[Interface]\n";}
    auto cameraHeight=GameplaySettings::Load(path).cameraHeight;
    assert(cameraHeight.enabled && cameraHeight.seconds==.08f);
    for(const auto& value:std::vector<std::pair<const char*,float>>{{"nan",.08f},{"inf",.08f},{"",.08f},{"bad",.08f},{"-10",.01f},{"10",.3f},{".12",.12f}}) {
        {std::ofstream f(path);f<<"[Interface]\nCameraHeightSmoothing=false\nCameraHeightSeconds="<<value.first<<"\n";}
        cameraHeight=GameplaySettings::Load(path).cameraHeight;
        assert(!cameraHeight.enabled && cameraHeight.seconds==value.second);
    }
    {std::ofstream f(path);f<<"[CombatVisuals]\n";}
    auto body=GameplaySettings::Load(path).combatVisuals;
    assert(body.bodyBlood && body.bodyBloodCount==6 && body.bodyBloodScale==.65f && body.bodyBloodSeconds==.35f && body.bodyBloodSpeed==110);
    {std::ofstream f(path);f<<"[CombatVisuals]\nBodyBlood=false\nBodyBloodCount=4\nBodyBloodScale=0.5\nBodyBloodSeconds=0.2\nBodyBloodSpeed=80\n";}
    body=GameplaySettings::Load(path).combatVisuals;
    assert(!body.bodyBlood && body.headshotBlood && body.bodyBloodCount==4 && body.bodyBloodScale==.5f && body.bodyBloodSeconds==.2f && body.bodyBloodSpeed==80);
    // Malformed body tuning falls back; finite outliers clamp before reaching the particle pool.
    for(const char* value:{"nan","inf","bad","","-1000","999999"}) {
        {std::ofstream f(path);f<<"[CombatVisuals]\nBodyBloodCount="<<value<<"\nBodyBloodScale="<<value<<"\nBodyBloodSeconds="<<value<<"\nBodyBloodSpeed="<<value<<"\n";}
        body=GameplaySettings::Load(path).combatVisuals;
        assert(body.bodyBloodCount>=1 && body.bodyBloodCount<=64 && body.bodyBloodScale>=.25f && body.bodyBloodScale<=4);
        assert(body.bodyBloodSeconds>=.1f && body.bodyBloodSeconds<=3 && body.bodyBloodSpeed>=0 && body.bodyBloodSpeed<=500);
        if(!strcmp(value,"nan") || !strcmp(value,"inf") || !strcmp(value,"bad") || !*value)
            assert(body.bodyBloodCount==6 && body.bodyBloodScale==.65f && body.bodyBloodSeconds==.35f && body.bodyBloodSpeed==110);
    }
    {std::ofstream f(path);f<<"[CombatVisuals]\nHeadshotBlood=false\nCriticalText=0\nShellCasings=false\nBloodCount=7\nBloodSeconds=1.2\nShellUp=-20\nShellSpeed=200\nShellMaximum=16\n";}
    auto cv=GameplaySettings::Load(path).combatVisuals;
    assert(!cv.headshotBlood && !cv.criticalText && !cv.shellCasings && cv.bloodCount==7 && cv.bloodSeconds==1.2f && cv.shellOffset[1]==-20 && cv.shellSpeed==200 && cv.shellMaximum==16);
    for(const char* value:{"nan","inf","bad","","-1000","999999"}) {
        {std::ofstream f(path);f<<"[CombatVisuals]\nBloodCount="<<value<<"\nBloodSeconds="<<value<<"\nShellGravity="<<value<<"\nShellSpeed="<<value<<"\nShellMaximum="<<value<<"\nShellUp="<<value<<"\nCriticalScale="<<value<<"\nShellBounce="<<value<<"\n";}
        cv=GameplaySettings::Load(path).combatVisuals;
        assert(cv.bloodCount>=1 && cv.bloodCount<=64 && cv.bloodSeconds>=.1f && cv.bloodSeconds<=3);
        assert(cv.shellGravity>=50 && cv.shellGravity<=2000 && cv.shellSpeed>=20 && cv.shellSpeed<=500 && cv.shellMaximum>=1 && cv.shellMaximum<=128);
        assert(cv.shellOffset[1]>=-64 && cv.shellOffset[1]<=64 && cv.criticalScale>=.5f && cv.criticalScale<=2 && cv.shellBounce>=0 && cv.shellBounce<=.9f);
        if(!strcmp(value,"nan") || !strcmp(value,"inf") || !strcmp(value,"bad") || !*value)assert(cv.shellOffset[1]==-12 && cv.shellSpeed==140 && cv.bloodSeconds==.8f);
    }
    // Startup/F8 migrates only former stock spray; explicit versioned old values and custom tuning survive.
    {std::ofstream f(path);f<<"[CombatVisuals]\nBloodCount=16\nBloodSeconds=0.5\nBloodScale=1\nBloodSpeed=180\n";}
    cv=GameplaySettings::Load(path).combatVisuals;
    assert(cv.bloodCount==48 && cv.bloodSeconds==.8f && cv.bloodScale==1.75f && cv.bloodSpeed==220);
    {std::ofstream f(path);f<<"[CombatVisuals]\nBloodVersion=2\nBloodCount=16\nBloodSeconds=0.5\nBloodScale=1\nBloodSpeed=180\n";}
    cv=GameplaySettings::Load(path).combatVisuals;
    assert(cv.bloodCount==16 && cv.bloodSeconds==.5f && cv.bloodScale==1 && cv.bloodSpeed==180);
    // Startup and F8 share this parser: head removal has its own opt-out and fatal spray stays bounded.
    for(const char* value:{"", "broken", "true", "false", "0", "1"}) {
        {std::ofstream f(path);f<<"[CombatVisuals]\nHeadshotDecapitation="<<value<<"\n";}
        cv=GameplaySettings::Load(path).combatVisuals;
        assert(cv.headshotDecapitation==(strcmp(value,"false") && strcmp(value,"0")));
        assert(cv.fatalBloodMultiplier==3);
    }
    for(const auto& value:std::vector<std::pair<const char*,float>>{
        {"nan",3},{"inf",3},{"broken",3},{"",3},{"-10",1},{"0",1},{"1.5",1.5f},{"999",4}}) {
        {std::ofstream f(path);f<<"[CombatVisuals]\nHeadshotBlood=false\nHeadshotDecapitation=false\nFatalBloodMultiplier="<<value.first<<"\n";}
        cv=GameplaySettings::Load(path).combatVisuals;
        assert(!cv.headshotBlood && !cv.headshotDecapitation && cv.fatalBloodMultiplier==value.second);
    }
    // Startup/F8 upgrades old stock and preserves custom, disabled and explicit newer-version values.
    for(const auto& tuning:std::vector<std::tuple<const char*,const char*,const char*,int,int,float,float>>{
        {"RPG","Damage","DamageVersion",2,WeaponSlots::RPG,100,1800},
        {"RPG","Radius","BlastVersion",3,-1,525,600},
        {"HEGrenade","Damage","DamageVersion",3,WeaponSlots::HE,300,600},
        {"M3","HeroMultiplier","MultiplierVersion",2,-2,3,6},
        {"XM1014","HeroMultiplier","MultiplierVersion",2,-3,2.4f,4.8f}}) {
        auto [section,key,marker,current,slot,oldValue,newValue]=tuning;
        for(int version:{1,current-1,current})for(float chosen:{-1.f,0.f,oldValue,777.f,2000000.f}) {
            {std::ofstream f(path);f<<"["<<section<<"]\n"<<key<<"="<<chosen<<"\n"<<marker<<"="<<version<<"\n";}
            auto balance=GameplaySettings::Load(path);
            float actual=slot>=0 ? balance.damage[slot] : slot==-1 ? balance.rpg.radius : balance.heroMultiplier[slot==-2 ? WeaponSlots::M3 : WeaponSlots::XM1014];
            float expected=chosen==oldValue && version<current ? newValue : chosen;
            expected=std::clamp(expected,slot==-1 ? 32.f : 0.f,slot>=0 ? 1000000.f : slot==-1 ? 3000.f : 1000.f);
            assert(std::abs(actual-expected)<.001f);
        }
        for(const char* invalid:{"nan","broken",""}) {
            {std::ofstream f(path);f<<"["<<section<<"]\n"<<key<<"="<<invalid<<"\n";}
            auto balance=GameplaySettings::Load(path);
            float actual=slot>=0 ? balance.damage[slot] : slot==-1 ? balance.rpg.radius : balance.heroMultiplier[slot==-2 ? WeaponSlots::M3 : WeaponSlots::XM1014];
            assert(std::abs(actual-newValue)<.001f);
        }
    }
    // Stock HE migrates once in the actual startup/F8 loader; custom, zero and versioned 100 remain intentional.
    for(const char* config:{"", "[HEGrenade]\nDamage=100\n", "[HEGrenade]\nDamage=100.0\nDamageVersion=1\n"}) {
        {std::ofstream f(path);f<<config;}assert(GameplaySettings::Load(path).damage[WeaponSlots::HE]==600);
    }
    for(int damage:{0,100,777}) {
        {std::ofstream f(path);f<<"[HEGrenade]\nDamage="<<damage<<"\nDamageVersion=2\n[Flashbang]\nStunAllUnits=false\nStunDistanceFalloff=true\n";}
        auto explosive=GameplaySettings::Load(path);assert(explosive.damage[WeaponSlots::HE]==damage);
        assert(!explosive.grenades.flashStunAllUnits && explosive.grenades.flashStunDistanceFalloff);
    }
    for(const char* value:{"", "broken", "true", "false", "0", "1"}) {
        {std::ofstream f(path);f<<"[Flashbang]\nStunAllUnits="<<value<<"\nStunDistanceFalloff="<<value<<"\n";}
        auto flash=GameplaySettings::Load(path).grenades;
        assert(flash.flashStunAllUnits==(strcmp(value,"false") && strcmp(value,"0")));
        assert(flash.flashStunDistanceFalloff==(!strcmp(value,"true") || !strcmp(value,"1")));
    }
    // New head/contact and campaign economy controls replace one validated startup/F8 snapshot.
    {std::ofstream f(path);f<<"[Combat]\nEnergyHeadshots=0\nHeadshotBoneWeight=nan\n[ArmyAI]\nShowStatus=false\nPlacementAttempts=999\nLumberWorkers=-1\n";}
    auto repairs=GameplaySettings::Load(path);
    assert(!repairs.hitDetection.energyHeadshots && repairs.hitDetection.headshotBoneWeight==.35f);
    assert(!repairs.armyAI.showStatus && repairs.armyAI.placementAttempts==32 && repairs.armyAI.lumberWorkers==0);
    {std::ofstream f(path);f<<"[Combat]\nHeadshotBoneWeight=0\n[ArmyAI]\nPlacementAttempts=0\nLumberWorkers=99\n";}
    repairs=GameplaySettings::Load(path);
    assert(repairs.hitDetection.energyHeadshots && repairs.hitDetection.headshotBoneWeight==.05f);
    assert(repairs.armyAI.showStatus && repairs.armyAI.placementAttempts==1 && repairs.armyAI.lumberWorkers==10);
    DeleteFileA(path.c_str());return 0;
}
