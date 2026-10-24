#pragma once
#include "WarcraftApi.hpp"
#include "../config/WeaponDropSettings.hpp"
#include <vector>

// Native effects mark local CS loot without introducing Warcraft items, inventory slots or pickup triggers.
class DroppedWeapons {
public:
    bool Spawn(int slot,float x,float y,const WeaponDropSettings& settings);
    void Tick(const WeaponDropSettings& settings,bool enabled);
    int Nearest(wc3::Handle actor,float radius) const;
    int Slot(int index) const {return drops_[index].slot;}
    void Take(int index);
    void Reset();
private:
    struct Drop {int slot;float x,y,expires;wc3::Handle marker;};
    float Time() const {return timer_ ? wc3::Real(wc3::TimerGetElapsed(timer_)) : 0;}
    std::vector<Drop> drops_;
    wc3::Handle timer_=0;
};
