#include "../src/config/GameplaySettings.hpp"
#include "../src/inventory/AmmoRecovery.hpp"
#include "../src/combat/CombatDamage.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
#include <windows.h>
namespace wc3 { void Log(const char*,...) {} }
int main() {
    GameplaySettings settings;
    assert(settings.ContactDamage(0,false,100)==36);
    assert(settings.ContactDamage(4,true,100)==65);
    // Default AWP hits use their configured base damage rather than health-dependent finishing damage.
    assert(!settings.FinishingAWP(3));
    assert(CombatDamage::Amount(settings.ContactDamage(3,false,100),false,
        CombatDamage::FinishesTarget(settings.FinishingAWP(3),false),5000)==115);
    assert(settings.friendlyFirePercent==50);
    assert(settings.floatingTextDistance==1200);
    assert(settings.csVolumePercent==100);
    assert(!settings.startAllWeapons && settings.startBombs==20 && settings.maxBombs==100 );
    assert(settings.buyAccess==GameplaySettings::BuyAccess::Anywhere && !settings.csSky);
    assert(settings.weaponPrice[0]==625 && settings.weaponPrice[1]==775 && settings.weaponPrice[2]==125);
    assert(settings.weaponPrice[3]==1188 && settings.weaponPrice[5]==50 && settings.weaponPrice[6]==250);
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
    char directory[MAX_PATH];GetCurrentDirectoryA(MAX_PATH,directory);
    std::string path=std::string(directory)+"\\settings-test.ini";
    // Exercise the actual INI boundary, including absent/invalid defaults and explicit opt-in/out reloads.
    const struct { const char* value; bool enabled; } awpCases[] = {
        {"",false},{"false",false},{"true",true},{"TrUe",true},{"0",false},{"1",true},{"broken",false}
    };
    int awpIndex=0;
    for (const auto& test:awpCases) {
        std::string sample=path+".awp-"+std::to_string(awpIndex++);
        { std::ofstream file(sample);file<<"[Damage]\nAWPOneShot="<<test.value<<"\n"; }
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
    settings=GameplaySettings::Load(path);assert(settings.runeAmmoPercent==20 && settings.runeAmmoAllWeapons && !settings.heroDamage);
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
    return 0;
}
