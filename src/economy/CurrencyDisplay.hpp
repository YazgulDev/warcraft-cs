#pragma once
#include "../config/EconomySettings.hpp"
#include <string>

namespace CurrencyDisplay {
// Prices, wallet totals and the play HUD share the active currency label.
inline std::string Format(int amount,const EconomySettings& settings) {
    return settings.currency==EconomySettings::Currency::Dollars ? "$"+std::to_string(amount) : std::to_string(amount)+" GOLD";
}
}
