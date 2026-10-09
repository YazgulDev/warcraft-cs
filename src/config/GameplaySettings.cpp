#include "GameplaySettings.hpp"
#include "../platform/WarcraftApi.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

GameplaySettings GameplaySettings::Load(const std::string& filename) {
    GameplaySettings result;
    auto text = [&](const char* section,const char* key,const char* fallback) {
        char value[128]; GetPrivateProfileStringA(section,key,fallback,value,sizeof(value),filename.c_str());
        return std::string(value);
    };
    auto number = [&](const char* section,const char* key,float fallback,float maximum) {
        std::string value=text(section,key,""); if (value.empty()) return fallback;
        char* end=nullptr; float parsed=std::strtof(value.c_str(),&end);
        // Reject malformed/NaN values rather than letting invalid config poison ammo, native damage or distances.
        if (end==value.c_str() || *end || !std::isfinite(parsed)) { wc3::Log("Invalid config %s.%s; using default",section,key);return fallback; }
        return std::clamp(parsed,0.0f,maximum);
    };
    // Bound diagnostic frequency/storage at the existing INI boundary, including malformed legacy values.
    auto detailed = text("Logging", "Detailed", "true");
    if (!_stricmp(detailed.c_str(), "false") || detailed == "0") result.logging.detailed = false;
    else if (_stricmp(detailed.c_str(), "true") && detailed != "1") wc3::Log("Invalid Logging.Detailed; using true");
    result.logging.intervalMs = unsigned(std::max(100.f, number("Logging", "IntervalMs", 1000, 60000)));
    result.logging.maxFileMB = unsigned(std::max(1.f, number("Logging", "MaxFileMB", 8, 64)));
    result.logging.archiveCount = unsigned(number("Logging", "ArchiveCount", 3, 8));
    result.runeAmmoPercent=number("Runes","AmmoPercent",20,100);
    // Missing settings enable the requested hopping; malformed switches preserve documented defaults.
    auto boolean = [&](const char* key, bool fallback) {
        auto value = text("Movement", key, fallback ? "true" : "false");
        if (!_stricmp(value.c_str(), "true")) return true;
        if (!_stricmp(value.c_str(), "false")) return false;
        return fallback;
    };
    result.movement.bunnyHop = boolean("BunnyHop", true);
    result.movement.autoJump = boolean("AutoJump", true);
    result.movement.jumpBoostPercent = number("Movement", "JumpBoostPercent", 0, 100);
    result.movement.maxBunnySpeed = std::max(250.0f, number("Movement", "MaxBunnySpeed", 1000, 2000));
    result.movement.airAcceleration = number("Movement", "AirAcceleration", 10, 100);
    result.movement.jumpSpeed = std::max(1.0f, number("Movement", "JumpSpeed", 268.328f, 800));
    result.movement.gravity = std::max(100.0f, number("Movement", "Gravity", 800, 3000));
    result.movement.stepHeight = number("Movement", "StepHeight", 27, 64);
    result.runePickupRadius=number("Runes","PickupRadius",160,400);
    result.runeAmmoAllWeapons=_stricmp(text("Runes","AmmoWeapons","all").c_str(),"current")!=0;
    result.heroDamage=_stricmp(text("Damage","Mode","weapon").c_str(),"hero")==0;
    // Missing or malformed settings must not silently enable instant kills; explicit preferences still reload on F8.
    auto awpOneShot=text("Damage","AWPOneShot","false");
    result.awpOneShot=!_stricmp(awpOneShot.c_str(),"true") || awpOneShot=="1";
    if (!awpOneShot.empty() && _stricmp(awpOneShot.c_str(),"true") && _stricmp(awpOneShot.c_str(),"false") &&
        awpOneShot!="0" && awpOneShot!="1") wc3::Log("Invalid Damage.AWPOneShot; using false");
    // Old configs retain 50%; zero disables allied damage and full damage is capped at 100%.
    result.friendlyFirePercent=number("Damage","FriendlyFirePercent",50,100);
    result.floatingTextDistance=number("Interface","FloatingTextDistance",1200,5000);
    // Legacy files keep their CS volume; malformed values cannot enter the native audio mixer.
    result.csVolumePercent=number("Audio","CSVolumePercent",100,100);
    // Keep formation spacing positive and leash beyond it so units can finish nearby fights.
    result.squadRadius=number("Squad","RecruitRadius",600,2000);
    result.squadFollowDistance=std::max(80.0f,number("Squad","FollowDistance",180,500));
    result.squadLeash=std::max(result.squadFollowDistance+200,number("Squad","CombatLeash",900,3000));
    result.squadMaxUnits=int(std::max(1.0f,number("Squad","MaxUnits",24,64)));
    // Restricted modes govern both opening B and every transaction, including the quick-ammo key.
    auto access=text("Buy","Access","anywhere");
    if (!_stricmp(access.c_str(),"friendly")) result.buyAccess=BuyAccess::FriendlyBuildings;
    else if (!_stricmp(access.c_str(),"shops")) result.buyAccess=BuyAccess::Shops;
    else if (_stricmp(access.c_str(),"anywhere")) { result.buyAccess=BuyAccess::Shops;wc3::Log("Invalid Buy.Access; using shops"); }
    result.buyRadius=number("Buy","Radius",600,3000);
    result.shopTypes=text("Buy","ShopTypes","");
    result.startAllWeapons=!_stricmp(text("Loadout","Mode","melee").c_str(),"all");
    result.maxBombs=int(number("Loadout","MaxBombs",100,1000));
    result.startBombs=std::min(result.maxBombs,int(number("Loadout","BombCount",20,1000)));
    result.csSky=_stricmp(text("Sky","Enabled","false").c_str(),"false")!=0;
    // Default to Warcraft's sky even for older configs missing this key; explicit CS sky still takes precedence.
    result.nativeSky=_stricmp(text("Sky","WarcraftEnabled","true").c_str(),"true")==0;
    // Only filename stems from private CS caches are accepted; INI strings never become arbitrary paths.
    auto sky=[&](const char* key,const char* fallback) {
        auto value=text("Sky",key,fallback);
        if (value.empty() || value.size()>48 || value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos)
            return std::string(fallback);
        return value;
    };
    result.defaultSky=sky("Default","Des");
    const char* tilesets="ALFWBYXVQJNDCIGOZ";
    for (const char* code=tilesets;*code;++code) {
        char key[]={*code,0};
        const char* fallback=strchr("NWIC",*code) ? "snow" : strchr("OD",*code) ? "DrkG" : strchr("AZJ",*code) ? "forest" : "Des";
        result.tilesetSky[static_cast<unsigned char>(*code)]=sky(key,fallback);
    }
    const char* sections[]={"AK47","M4A1","USP","AWP","Knife","C4","Sword"};
    for (int slot=0;slot<WeaponSlots::Count;++slot) {
        result.damage[slot]=number(sections[slot],"Damage",result.damage[slot],1000000);
        result.heroMultiplier[slot]=number(sections[slot],"HeroMultiplier",result.heroMultiplier[slot],1000);
        result.weaponPrice[slot]=int(number(sections[slot],"Price",float(result.weaponPrice[slot]),1000000));
        result.ammoPrice[slot]=int(number(sections[slot],"AmmoPrice",float(result.ammoPrice[slot]),1000000));
        result.ammoPack[slot]=int(std::max(1.0f,number(sections[slot],"AmmoPack",float(std::max(1,result.ammoPack[slot])),1000)));
    }
    result.knifeSecondaryDamage=number("Knife","SecondaryDamage",65,1000000);
    result.swordSecondaryDamage=number("Sword","SecondaryDamage",240,1000000);
    return result;
}
float GameplaySettings::ContactDamage(int slot,bool secondary,float heroAttack) const {
    if (slot<0 || slot>=WeaponSlots::Count) return 0;
    // Native average attack works for heroes and creeps, including their active bonuses; heavy melee keeps its ratio.
    float heavy=secondary ? (slot==WeaponSlots::Knife ? 65.0f/40 : slot==WeaponSlots::Sword ? 2.0f : 1.0f) : 1.0f;
    if (heroDamage && slot!=WeaponSlots::C4) return std::max(0.0f,heroAttack)*heroMultiplier[slot]*heavy;
    return secondary && slot==WeaponSlots::Knife ? knifeSecondaryDamage :
        secondary && slot==WeaponSlots::Sword ? swordSecondaryDamage : damage[slot];
}
