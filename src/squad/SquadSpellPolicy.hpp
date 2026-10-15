#pragma once
#include <cmath>

// Tactical roles are explicit: generic target-form probing must never turn a heal into friendly damage.
enum class SquadSpellRole { Enemy, Point, Area, Heal, Buff, Summon };
namespace SquadSpellPolicy {
inline bool Ready(int level,bool disabled,bool noMana,float cooldown) {return level>0 && !disabled && !noMana && cooldown<=.05f;}
inline bool NeedsHealing(float health,float maximum,float threshold) {
    return std::isfinite(health) && std::isfinite(maximum) && maximum>0 && health>.405f && health*100/maximum<threshold;
}
}
