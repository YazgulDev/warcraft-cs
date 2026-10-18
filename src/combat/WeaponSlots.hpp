#pragma once

// Stable inventory/cache IDs survive category selection and the optional legacy shortcut layout.
namespace WeaponSlots {
constexpr int Count = 17;
constexpr int ShortcutCount = 7; // Menu groups reuse bindings 1..5; legacy direct IDs remain available.
constexpr int Pistol = 2;
constexpr int AWP = 3;
constexpr int Knife = 4;
constexpr int C4 = 5;
constexpr int Sword = 6;
constexpr int Deagle=7,Glock=8,MP5=9,P90=10,M249=11,M3=12,XM1014=13;
constexpr int HE=14,Flashbang=15,Smoke=16;
inline constexpr const char* Caches[]={"ak47","m4a1","usp","awp","knife","c4","sword","deagle","glock18","mp5","p90","m249","m3","xm1014","hegrenade","flashbang","smokegrenade"};
inline constexpr const char* Sections[]={"AK47","M4A1","USP","AWP","Knife","C4","Sword","Deagle","Glock","MP5","P90","M249","M3","XM1014","HEGrenade","Flashbang","SmokeGrenade"};
inline bool Melee(int index) { return index == Knife || index == Sword; }
// Consumable throws share inventory slots, never the bullet/reload/muzzle paths.
inline bool Grenade(int index) {return index>=HE && index<=Smoke;}
inline bool Firearm(int index) {return index>=0 && index<Count && !Melee(index) && index!=C4 && !Grenade(index);}
}
