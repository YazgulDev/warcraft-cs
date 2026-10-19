#pragma once
#include "WarcraftApi.hpp"
#include "../combat/GrenadeProjectile.hpp"
#include <functional>

// Shared HE/rocket damage adapter preserves map callbacks, cover and native quest immunity.
namespace NativeBlast {
bool Covered(const GrenadePoint& from,const GrenadePoint& to,const std::vector<Bounds3>& obstacles);
bool Apply(wc3::Handle actor,int type,const GrenadePoint& from,float damage,float radius,float friendly,
    const std::vector<Bounds3>& obstacles,const std::function<bool()>& valid,const char* label);
}
