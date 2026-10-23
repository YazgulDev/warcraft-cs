#pragma once

// The CS wallet is independent of map resources; F8 changes policy without granting starting money again.
struct EconomySettings {
    enum class Currency { Dollars, Gold };
    Currency currency=Currency::Dollars;
    int startingMoney=800,killReward=300,maximumMoney=16000;
};
