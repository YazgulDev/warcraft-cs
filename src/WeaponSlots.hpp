#pragma once

// Stable number keys keep the original five weapons while extending every cache together.
namespace WeaponSlots {
constexpr int Count = 7;
constexpr int Knife = 4;
constexpr int C4 = 5;
constexpr int Sword = 6;
inline bool Melee(int index) { return index == Knife || index == Sword; }
}
