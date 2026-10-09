#pragma once
#include "PlantedBomb.hpp"
#include <vector>
#include <algorithm>

// Independent native clocks keep every planted C4 alive across weapon switches and FPS exits.
class BombField {
public:
    bool Plant(wc3::Handle actor,float x,float y,GameAudio& audio,float damage,float friendly) {
        if (bombs_.size()>=1000) return false;
        PlantedBomb charge;
        if (!charge.Plant(actor,x,y,audio,damage,friendly)) return false;
        bombs_.push_back(charge);return true;
    }
    bool Tick(GameAudio& audio) {
        bool hit=false;for (auto& charge:bombs_) hit=charge.Tick(audio)||hit;
        bombs_.erase(std::remove_if(bombs_.begin(),bombs_.end(),[](const PlantedBomb& charge){return charge.Finished();}),bombs_.end());
        return hit;
    }
    float Remaining() const {
        float remaining=-1;
        for (const auto& charge:bombs_) if (charge.Active() && (remaining<0 || charge.Remaining()<remaining)) remaining=charge.Remaining();
        return remaining;
    }
    size_t Count() const {
        return std::count_if(bombs_.begin(),bombs_.end(),[](const PlantedBomb& charge){return charge.Active();});
    }
    // Map unload owns destruction of native handles; forgetting them must not touch the next map.
    void Reset() { bombs_.clear(); }
private:
    std::vector<PlantedBomb> bombs_;
};
