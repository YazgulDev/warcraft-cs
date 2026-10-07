#include "GameplaySettings.hpp"
#include "WarcraftApi.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

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
    result.runeAmmoPercent=number("Runes","AmmoPercent",20,100);
    result.runePickupRadius=number("Runes","PickupRadius",160,400);
    result.runeAmmoAllWeapons=_stricmp(text("Runes","AmmoWeapons","all").c_str(),"current")!=0;
    result.heroDamage=_stricmp(text("Damage","Mode","weapon").c_str(),"hero")==0;
    result.awpOneShot=_stricmp(text("Damage","AWPOneShot","true").c_str(),"false")!=0;
    // Old configs retain 50%; zero disables allied damage and full damage is capped at 100%.
    result.friendlyFirePercent=number("Damage","FriendlyFirePercent",50,100);
    // Keep formation spacing positive and leash beyond it so units can finish nearby fights.
    result.squadRadius=number("Squad","RecruitRadius",600,2000);
    result.squadFollowDistance=std::max(80.0f,number("Squad","FollowDistance",180,500));
    result.squadLeash=std::max(result.squadFollowDistance+200,number("Squad","CombatLeash",900,3000));
    result.squadMaxUnits=int(std::max(1.0f,number("Squad","MaxUnits",24,64)));
    const char* sections[]={"AK47","M4A1","USP","AWP","Knife","C4","Sword"};
    for (int slot=0;slot<WeaponSlots::Count;++slot) {
        result.damage[slot]=number(sections[slot],"Damage",result.damage[slot],1000000);
        result.heroMultiplier[slot]=number(sections[slot],"HeroMultiplier",result.heroMultiplier[slot],1000);
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
