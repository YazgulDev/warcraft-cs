#pragma once

// The CS wallet is independent of map resources; F8 changes policy without granting starting money again.
struct EconomySettings {
    enum class Currency { Dollars, Gold };
    Currency currency=Currency::Dollars;
    // Confirmed hostile creature kills pay $200 by default; F8 can apply a custom reward.
    int startingMoney=800,killReward=200,maximumMoney=16000;
};
