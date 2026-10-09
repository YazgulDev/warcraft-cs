#pragma once
#include "../config/GameplaySettings.hpp"
#include "../platform/WarcraftApi.hpp"

namespace BuyAccess {
// Re-evaluate native alliances/shops at purchase time, so leaving a shop or losing it blocks payment.
bool Allowed(wc3::Handle actor,const GameplaySettings& settings);
}
