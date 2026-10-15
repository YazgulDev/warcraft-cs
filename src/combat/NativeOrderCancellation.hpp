#pragma once
#include <cstdint>
#include <unordered_map>

// Native stop can retain an attack target. Cancel each acquisition once instead of resetting manual swings every tick.
class NativeOrderCancellation {
public:
    bool Observe(uint32_t actor,uintptr_t target,int order) {
        // Squad followers share this policy; their observations cannot erase the controlled hero's retained target.
        if(targets_.size()>1024)targets_.clear();
        auto previous=targets_.find(actor);
        bool acquired=target && (previous==targets_.end() || previous->second!=target);
        if(target)targets_[actor]=target;else targets_.erase(actor);
        return (order && order!=851972) || acquired;
    }
private:
    std::unordered_map<uint32_t,uintptr_t> targets_;
};
