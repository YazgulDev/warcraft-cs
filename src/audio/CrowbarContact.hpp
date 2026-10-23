#pragma once

// Warcraft structures are units too; classify the physical contact rather than treating every unit as flesh.
namespace CrowbarContact {
enum class Surface {Creature,Structure,Destructable,Ground};
inline const char* Name(Surface surface) {
    return surface==Surface::Creature ? "creature" : surface==Surface::Structure ? "structure" : surface==Surface::Destructable ? "destructable" : "ground";
}
inline const char* Sample(Surface surface,unsigned serial,bool varied=true) {
    static const char* body[]={"hl_cbar_hitbod1.wav","hl_cbar_hitbod2.wav","hl_cbar_hitbod3.wav"};
    static const char* hard[]={"hl_cbar_hit1.wav","hl_cbar_hit2.wav"};
    static const char* ground[]={"hl_cbar_ground1.wav","hl_cbar_ground2.wav","hl_cbar_ground3.wav"};
    if(!varied)return surface==Surface::Creature ? body[0] : hard[0];
    return surface==Surface::Creature ? body[serial%3] : surface==Surface::Ground ? ground[serial%3] : hard[serial%2];
}
}
