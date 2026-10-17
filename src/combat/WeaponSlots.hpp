#pragma once

// Stable number keys keep the original five weapons while extending every cache together.
namespace WeaponSlots {
constexpr int Count = 14;
constexpr int ShortcutCount = 7; // Extra purchases use the wheel; existing 1..7 bindings keep their IDs.
constexpr int Pistol = 2;
constexpr int AWP = 3;
constexpr int Knife = 4;
constexpr int C4 = 5;
constexpr int Sword = 6;
constexpr int Deagle=7,Glock=8,MP5=9,P90=10,M249=11,M3=12,XM1014=13;
inline constexpr const char* Caches[]={"ak47","m4a1","usp","awp","knife","c4","sword","deagle","glock18","mp5","p90","m249","m3","xm1014"};
inline constexpr const char* Sections[]={"AK47","M4A1","USP","AWP","Knife","C4","Sword","Deagle","Glock","MP5","P90","M249","M3","XM1014"};
inline bool Melee(int index) { return index == Knife || index == Sword; }
inline bool Firearm(int index) {return index>=0 && index<Count && !Melee(index) && index!=C4;}
}
