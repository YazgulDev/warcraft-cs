#pragma once
#include "WeaponTrace.hpp"
#include "../config/ItemHoverSettings.hpp"
#include <map>
#include <array>

// Native queries stay on the game thread; renderers consume only the copied label snapshot.
class ItemHover {
public:
    void Reset(){Clear();items_.clear();names_.clear();actor_=0;scanned_=0;}
    void Clear(){item_=0;name_.clear();}
    wc3::Handle Item() const {return item_;}
    const std::array<float,3>& Position() const {return position_;}
    const std::string& Name() const {return name_;}
    void Update(wc3::Handle actor,const float* origin,const float* direction,const GameplaySettings& settings,
        UnitHitboxes& units,DestructableHitboxes& objects,uintptr_t base);
private:
    wc3::Handle actor_=0,item_=0;
    DWORD scanned_=0;
    float scanX_=0,scanY_=0;
    std::vector<wc3::Handle> items_;
    std::map<int,std::string> names_;
    std::string name_;
    std::array<float,3> position_={};
};
