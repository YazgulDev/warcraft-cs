#pragma once

// Stable inventory/cache IDs survive category selection and the optional legacy shortcut layout.
namespace WeaponSlots {
constexpr int Count = 21;
constexpr int SelectionGroups = 6; // Half-Life has its own carried-weapon group on key 6.
constexpr int ShortcutCount = 7; // Menu groups reuse bindings 1..6; legacy direct IDs remain available.
constexpr int Pistol = 2;
constexpr int AWP = 3;
constexpr int Knife = 4;
constexpr int C4 = 5;
constexpr int Sword = 6;
constexpr int Deagle=7,Glock=8,MP5=9,P90=10,M249=11,M3=12,XM1014=13;
constexpr int HE=14,Flashbang=15,Smoke=16;
constexpr int RPG=17; // Optional Half-Life content appends an ID without moving existing weapons.
constexpr int Crowbar=18,Tau=19,Gluon=20;
inline constexpr const char* Caches[]={"ak47","m4a1","usp","awp","knife","c4","sword","deagle","glock18","mp5","p90","m249","m3","xm1014","hegrenade","flashbang","smokegrenade","rpg","crowbar","gauss","egon"};
inline constexpr const char* Sections[]={"AK47","M4A1","USP","AWP","Knife","C4","Sword","Deagle","Glock","MP5","P90","M249","M3","XM1014","HEGrenade","Flashbang","SmokeGrenade","RPG","Crowbar","TauCannon","GluonGun"};
inline bool HalfLife(int index) {return index>=RPG && index<Count;}
inline bool Energy(int index) {return index==Tau || index==Gluon;}
inline bool Melee(int index) { return index == Knife || index == Sword || index==Crowbar; }
// Consumable throws share inventory slots, never the bullet/reload/muzzle paths.
inline bool Grenade(int index) {return index>=HE && index<=Smoke;}
inline bool Firearm(int index) {return index>=0 && index<Count && !Melee(index) && index!=C4 && !Grenade(index);}
}
