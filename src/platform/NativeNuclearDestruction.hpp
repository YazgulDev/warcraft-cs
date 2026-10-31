#pragma once
#include "KillReward.hpp"
#include "../config/NuclearSettings.hpp"
#include "../combat/GrenadeProjectile.hpp"
namespace NativeNuclearDestruction {
void Apply(wc3::Handle actor,int type,uintptr_t object,wc3::Handle owner,const GrenadePoint& center,const NuclearSettings& settings,
    const std::function<bool()>& valid,const KillReward::Sink& rewards);
}
