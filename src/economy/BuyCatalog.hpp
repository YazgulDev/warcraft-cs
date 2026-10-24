#pragma once
#include "../combat/WeaponSlots.hpp"
#include <array>

// One catalog supplies both number-key purchases and CS-style weapon/detail panels.
namespace BuyCatalog {
enum class Category { Main, Pistols, Shotguns, SMGs, Rifles, Machineguns, Equipment, HalfLife };
struct Entry { int slot,key;Category category;const char* name;const char* cache;const char* caliber; };
inline constexpr std::array<Entry,WeaponSlots::Count> Entries={{
    {0,1,Category::Rifles,"CV-47 / AK47","ak47","7.62 MM"},
    {1,2,Category::Rifles,"MAVERICK M4A1 CARBINE","m4a1","5.56 MM"},
    {2,1,Category::Pistols,"K&M .45 TACTICAL / USP","usp",".45 ACP"},
    {3,3,Category::Rifles,"MAGNUM SNIPER RIFLE / AWP","awp",".338 LAPUA MAGNUM"},
    {WeaponSlots::Knife,1,Category::Equipment,"KNIFE","knife","MELEE"},
    {WeaponSlots::C4,3,Category::Equipment,"C4 EXPLOSIVE","c4","EXPLOSIVE"},
    {WeaponSlots::Sword,2,Category::Equipment,"GREATSWORD","sword","MELEE"},
    {WeaponSlots::Deagle,2,Category::Pistols,"DESERT EAGLE","deagle",".50 AE"},
    {WeaponSlots::Glock,3,Category::Pistols,"GLOCK 18","glock18","9 MM"},
    {WeaponSlots::MP5,1,Category::SMGs,"K&M MP5","mp5","9 MM"},
    {WeaponSlots::P90,2,Category::SMGs,"FN P90","p90","5.7 MM"},
    {WeaponSlots::M249,1,Category::Machineguns,"M249 PARA","m249","5.56 MM"},
    {WeaponSlots::M3,1,Category::Shotguns,"BENELLI M3","m3","12 GAUGE"},
    {WeaponSlots::XM1014,2,Category::Shotguns,"BENELLI XM1014","xm1014","12 GAUGE"},
    {WeaponSlots::HE,4,Category::Equipment,"HE GRENADE","hegrenade","EXPLOSIVE"},
    {WeaponSlots::Flashbang,5,Category::Equipment,"FLASHBANG","flashbang","FLASH"},
    {WeaponSlots::Smoke,6,Category::Equipment,"SMOKE GRENADE","smokegrenade","SMOKE"},
    {WeaponSlots::Crowbar,1,Category::HalfLife,"CROWBAR","crowbar","MELEE"},
    {WeaponSlots::Tau,2,Category::HalfLife,"TAU CANNON","gauss","URANIUM"},
    {WeaponSlots::Gluon,3,Category::HalfLife,"GLUON GUN","egon","URANIUM"},
    {WeaponSlots::RPG,4,Category::HalfLife,"HALF-LIFE RPG","rpg","ROCKET"}
}};
inline const Entry* Find(Category category,int key) {
    for (const auto& item : Entries) if (item.category==category && item.key==key) return &item;
    return nullptr;
}
inline const Entry* Slot(int slot) {
    for (const auto& item : Entries) if (item.slot==slot) return &item;
    return nullptr;
}
}
