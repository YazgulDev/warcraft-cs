#pragma once
#include "WarcraftApi.hpp"

// Adapt a confirmed FPS contact to the unit's own attack callbacks, without issuing a chase order.
namespace NativeWeaponAttack {
using HookInstaller=bool (*)(void*,void*,void**);
void Install(uintptr_t base,HookInstaller hook,wc3::Handle (*controlledActor)()=nullptr,
    bool (*passiveUnit)(uintptr_t)=nullptr,bool (*passiveAttack)(uintptr_t)=nullptr);
bool Apply(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance);
// TPS ranged contacts keep the native projectile for units and destructable gates; FPS contacts use instant impact.
bool Launch(wc3::Handle attacker,wc3::Handle target,float damage,float contactDistance);
int PrimaryKind(wc3::Handle attacker);
float PrimaryRange(wc3::Handle attacker);
int AttackType(wc3::Handle attacker);
}
