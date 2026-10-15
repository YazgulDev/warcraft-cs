#pragma once
#include "WarcraftApi.hpp"

// Intercept native basic-attack targets and dispatch before pursuit, preserving saved counters and ranges.
namespace FpsCombatGuard {
using HookInstaller=bool (*)(void*,void*,void**);
void Install(uintptr_t base,wc3::Handle (*actor)(),bool (*passiveUnit)(uintptr_t),bool (*passiveAttack)(uintptr_t),HookInstaller hook,wc3::Handle (*protectedActor)()=nullptr);
// Hostile contacts against F9's opt-in protected actor; allies and other targets keep native rules.
bool BlocksIncomingAttack(uintptr_t attack,uintptr_t target);
void CancelOrders(wc3::Handle unit);
void CancelAttacks(wc3::Handle unit);
bool HasAttackTarget(wc3::Handle unit);
}
