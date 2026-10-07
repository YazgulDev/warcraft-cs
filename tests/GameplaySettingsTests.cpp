#include "../src/GameplaySettings.hpp"
#include "../src/AmmoRecovery.hpp"
#include "../src/CombatDamage.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
#include <windows.h>
namespace wc3 { void Log(const char*,...) {} }
int main() {
    GameplaySettings settings;
    assert(settings.ContactDamage(0,false,100)==36);
    assert(settings.ContactDamage(4,true,100)==65);
    assert(settings.FinishingAWP(3));
    assert(settings.friendlyFirePercent==50);
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
    // Invalid formation values cannot produce zero slots or a combat leash shorter than the spacing.
    { std::ofstream file(path);file<<"[Runes]\nAmmoPercent=35\nAmmoWeapons=current\nPickupRadius=999\n[Damage]\nMode=hero\n[AK47]\nHeroMultiplier=2.5\nDamage=nan\n[USP]\nDamage=broken\n[Squad]\nRecruitRadius=9999\nMaxUnits=0\nFollowDistance=0\nCombatLeash=0\n"; }
    settings=GameplaySettings::Load(path);DeleteFileA(path.c_str());
    assert(settings.runeAmmoPercent==35 && !settings.runeAmmoAllWeapons && settings.runePickupRadius==400);
    assert(settings.heroDamage && settings.heroMultiplier[0]==2.5f);
    assert(settings.damage[0]==36 && settings.damage[2]==34);
    assert(settings.squadRadius==2000 && settings.squadMaxUnits==1 && settings.squadFollowDistance==80 && settings.squadLeash==280);
    settings=GameplaySettings::Load(path);assert(settings.runeAmmoPercent==20 && settings.runeAmmoAllWeapons && !settings.heroDamage);
    assert(settings.friendlyFirePercent==50); // Missing keys keep legacy 50% damage.
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
    return 0;
}
