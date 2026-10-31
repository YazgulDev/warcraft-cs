#pragma once
#include "WarcraftApi.hpp"

// Intercept native basic-attack targets and dispatch before pursuit, preserving saved counters and ranges.
namespace FpsCombatGuard {
using HookInstaller=bool (*)(void*,void*,void**);
void Install(uintptr_t base,wc3::Handle (*actor)(),bool (*passiveUnit)(uintptr_t),bool (*passiveAttack)(uintptr_t),HookInstaller hook,wc3::Handle (*protectedActor)()=nullptr,bool (*blockedSight)(uintptr_t,uintptr_t)=nullptr);
// Hostile contacts against F9's opt-in protected actor; allies and other targets keep native rules.
// Scope only the manual shooter's synchronous native contact; nested attacks keep their own sight policy.
uintptr_t PermitBlindContact(uintptr_t attack);
bool BlocksUnseenTarget(uintptr_t attack,uintptr_t target);
bool BlocksIncomingAttack(uintptr_t attack,uintptr_t target);
void CancelOrders(wc3::Handle unit);
void CancelAttacks(wc3::Handle unit);
bool HasAttackTarget(wc3::Handle unit);
// Read-only identity lets temporary squad orders yield to newer native targets without canceling them.
uintptr_t AttackTargetObject(wc3::Handle unit);
}
