#pragma once
#include "../economy/BuyMenu.hpp"
#include "../config/GameplaySettings.hpp"

// Rendering receives a copied interaction snapshot, never native wallets or purchase methods.
struct BuyMenuState {
    explicit BuyMenuState(const GameplaySettings& value):settings(value){}
    const GameplaySettings& settings;
    BuyMenu::Page page=BuyMenu::Page::Closed;
    EconomySettings::Currency currency=EconomySettings::Currency::Dollars;
    int dollars=0,gold=0;
    float attackDamage=0,attackSpeed=1;
    std::array<bool,WeaponSlots::Count> owned{};
    std::string buyKey="B",ammoKey=".",refillKey="F11",allWeaponsKey="F12";
};
