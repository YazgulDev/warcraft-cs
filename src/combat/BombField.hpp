#pragma once
#include "PlantedBomb.hpp"
#include <vector>
#include <algorithm>
#include <iterator>

// Independent native clocks keep every planted C4 alive across weapon switches and FPS exits.
class BombField {
public:
    bool Plant(wc3::Handle actor,float x,float y,GameAudio& audio,float damage,float friendly,const BlastJumpSettings& jump={}) {
        if (bombs_.size()+processing_>=1000) return false;
        PlantedBomb charge;
        if (!charge.Plant(actor,x,y,audio,damage,friendly,jump)) return false;
        bombs_.push_back(charge);return true;
    }
    bool Tick(GameAudio& audio,const KillReward::Sink& onKill={},BlastJumpEvents* jumps=nullptr) {
        // Callbacks may Reset/Plant this field. Tick an owned snapshot, and never put old
        // native timer handles back into a replaced map or invalidate an active iterator.
        unsigned generation=generation_;auto pending=std::move(bombs_);bombs_.clear();processing_=pending.size();
        struct Release {size_t& count;~Release(){count=0;}} release{processing_};
        std::vector<PlantedBomb> survivors;bool hit=false;
        for(auto& charge:pending) {
            hit=charge.Tick(audio,onKill,jumps,[&]{return generation_==generation;})||hit;
            if(generation_!=generation)return hit;
            if(!charge.Finished())survivors.push_back(std::move(charge));
        }
        survivors.insert(survivors.end(),std::make_move_iterator(bombs_.begin()),std::make_move_iterator(bombs_.end()));
        bombs_=std::move(survivors);
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
    void Reset() { ++generation_;processing_=0;bombs_.clear(); }
private:
    std::vector<PlantedBomb> bombs_;
    unsigned generation_=0;
    size_t processing_=0;
};
