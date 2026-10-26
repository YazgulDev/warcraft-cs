#pragma once
#include "../config/EconomySettings.hpp"

// A purchase spends one complete currency; insufficient dollars never consume a partial wallet.
struct PurchasePayment {
    EconomySettings::Currency currency=EconomySettings::Currency::Dollars;
    int balance=0;
    bool fallback=false;
    static PurchasePayment Choose(EconomySettings::Currency preferred,int dollars,int gold,int cost,bool allowGold) {
        if(preferred==EconomySettings::Currency::Gold)return {preferred,gold,false};
        if(dollars<cost && allowGold && gold>=cost)return {EconomySettings::Currency::Gold,gold,true};
        return {preferred,dollars,false};
    }
};
