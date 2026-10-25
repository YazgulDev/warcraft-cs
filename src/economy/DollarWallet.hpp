#pragma once
#include "../config/EconomySettings.hpp"
#include <algorithm>
#include <cstdint>
#include <unordered_map>

// Wallets belong to players for one map; unit/view changes and F8 never mint a second starting balance.
class DollarWallet {
public:
    int Balance(uint32_t owner,const EconomySettings& settings) {
        auto found=balances_.emplace(owner,std::min(settings.startingMoney,settings.maximumMoney)).first;
        return found->second=std::min(found->second,settings.maximumMoney);
    }
    void Store(uint32_t owner,int balance,const EconomySettings& settings) {balances_[owner]=std::clamp(balance,0,settings.maximumMoney);}
    int Reward(uint32_t owner,const EconomySettings& settings) {
        int before=Balance(owner,settings);Store(owner,before+settings.killReward,settings);return Balance(owner,settings)-before;
    }
    void Reset() {balances_.clear();}
private:
    std::unordered_map<uint32_t,int> balances_;
};
