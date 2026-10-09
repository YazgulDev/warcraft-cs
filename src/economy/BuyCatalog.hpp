#pragma once
#include "../combat/WeaponSlots.hpp"
#include <array>

// One catalog supplies both number-key purchases and CS-style weapon/detail panels.
namespace BuyCatalog {
enum class Category { Main, Pistols, Rifles, Equipment };
struct Entry { int slot,key;Category category;const char* name;const char* cache;const char* caliber; };
inline constexpr std::array<Entry,WeaponSlots::Count> Entries={{
    {0,1,Category::Rifles,"CV-47 / AK47","ak47","7.62 MM"},
    {1,2,Category::Rifles,"MAVERICK M4A1 CARBINE","m4a1","5.56 MM"},
    {2,1,Category::Pistols,"K&M .45 TACTICAL / USP","usp",".45 ACP"},
    {3,3,Category::Rifles,"MAGNUM SNIPER RIFLE / AWP","awp",".338 LAPUA MAGNUM"},
    {WeaponSlots::Knife,1,Category::Equipment,"KNIFE","knife","MELEE"},
    {WeaponSlots::C4,3,Category::Equipment,"C4 EXPLOSIVE","c4","EXPLOSIVE"},
    {WeaponSlots::Sword,2,Category::Equipment,"GREATSWORD","sword","MELEE"}
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
