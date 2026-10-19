#pragma once
#include "Weapon.hpp"
#include "WeaponSlots.hpp"
#include <array>

// Shared defaults are copied into a validated F8 snapshot; stable slot IDs own every table.
namespace WeaponDefaults {
inline const std::array<Weapon,WeaponSlots::Count> Weapons={{
    {"AK47", "ak47-1.wav", 30, 90, 36, 0.100f, 2.45f, 3000, true},
    {"M4A1", "m4a1_unsil-1.wav", 30, 90, 33, 0.0875f, 3.05f, 3000, true}, // Original CS firing interval.
    {"USP", "usp_unsil-1.wav", 12, 100, 34, 0.150f, 2.70f, 2200, false},
    {"AWP", "awp1.wav", 10, 30, 115, 1.45f, 2.90f, 4000, false},
    {"KNIFE", "knife_slash1.wav", 0, 0, 40, 0.50f, 0, 150, false},
    // C4 occupies slot 6; the sword has no ammunition and reaches farther than the knife.
    {"C4", "c4_plant.wav", 1, 0, 2500, 0, 0, 800, false},
    {"GREATSWORD", "knife_slash1.wav", 0, 0, 120, 1.84f, 0, 230, false},
    // Appended slots preserve existing inventory IDs and share native contact effects.
    {"DEAGLE", "deagle-1.wav", 7, 35, 54, .225f, 2.2f, 3000, false},
    {"GLOCK", "glock18-2.wav", 20, 120, 25, .15f, 2.2f, 2200, false},
    {"MP5", "mp5-1.wav", 30, 120, 26, .075f, 2.6f, 2500, true},
    {"P90", "p90-1.wav", 50, 100, 21, .07f, 3.4f, 2500, true},
    {"M249", "m249-1.wav", 100, 200, 32, .08f, 4.7f, 3000, true},
    {"M3", "m3-1.wav", 8, 32, 104, .875f, 4.5f, 1200, false},
    {"XM1014", "xm1014-1.wav", 7, 32, 80, .25f, 3.5f, 1200, true},
    {"HE GRENADE","hegrenade-1.wav",1,0,100,0,0,350,false},
    {"FLASHBANG","flashbang-1.wav",2,0,0,0,0,1000,false},
    {"SMOKE GRENADE","sg_explode.wav",1,0,0,0,0,300,false},
    {"RPG","hl_rpg_fire.wav",1,5,100,1.5f,2,30000,false}
}};
}
