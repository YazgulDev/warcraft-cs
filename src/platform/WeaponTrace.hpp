#pragma once
#include "../combat/UnitHitboxes.hpp"
#include "../combat/DestructableHitboxes.hpp"
#include "../config/GameplaySettings.hpp"

// A read-only contact query shared by firearm damage and reticle item occlusion.
namespace WeaponTrace {
struct Options {bool volumes=false,includeAllies=false,surfaceOnMiss=false;float padding=0;bool diagnostic=false;};
struct Contact {wc3::Handle target=0;float distance=0;bool destructable=false,head=false;};
Contact Cast(wc3::Handle actor,const float* origin,const float* direction,float distance,
    const GameplaySettings& settings,UnitHitboxes& units,DestructableHitboxes& objects,uintptr_t base,Options options);
}
