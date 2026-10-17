#include "../src/config/GameplaySettings.hpp"
#include "MovementStrafeInput.hpp"
#include "../src/inventory/AmmoRecovery.hpp"
#include "../src/combat/CombatDamage.hpp"
#include "../src/combat/UnitScaling.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
#include <windows.h>
namespace wc3 { void Log(const char*,...) {} }
int main() {
    GameplaySettings settings;
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
    assert(!settings.logging.detailed && settings.logging.intervalMs==100 && settings.logging.maxFileMB==64 && settings.logging.archiveCount==0);
    WritePrivateProfileStringA("Logging","Detailed","true",path.c_str());
    WritePrivateProfileStringA("Logging","IntervalMs","2500",path.c_str());
    settings=GameplaySettings::Load(path); assert(settings.logging.detailed && settings.logging.intervalMs==2500);
    { std::ofstream file(path); file << "[Logging]\nDetailed=invalid\nIntervalMs=nan\nMaxFileMB=broken\nArchiveCount=inf\n"; }
    settings=GameplaySettings::Load(path);
    assert(settings.logging.detailed && settings.logging.intervalMs==1000 && settings.logging.maxFileMB==8 && settings.logging.archiveCount==3);
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
    // New squad parameters use the real INI boundary; live snapshots clamp invalid distances/refresh rates.
    assert(ShortcutBindings{}.Key(ShortcutAction::SquadOrders)=='Y');
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
    DeleteFileA(path.c_str());return 0;
}
