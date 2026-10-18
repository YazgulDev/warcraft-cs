#pragma once
#include <algorithm>
#include <cstdint>

// Each deliberate secondary press owns a bounded burst; cancellation never queues delayed shots on resume.
class BurstFire {
public:
    bool Ready(uint32_t now) const { return !remaining_ && (!cooling_ || int32_t(now-ready_)>=0); }
    bool Active() const { return remaining_>0; }
    bool Due(uint32_t now) const { return Active() && int32_t(now-next_)>=0; }
    void Begin(uint32_t now,int roundsAfterFirst,uint32_t interval,uint32_t cooldown) {
        remaining_=std::max(0,roundsAfterFirst);interval_=std::max(1u,interval);
        next_=now+interval_;ready_=now+cooldown;cooling_=true;
    }
    void Fired(uint32_t now) {
        // A slow frame releases at most one round, preserving spacing instead of dumping overdue rounds together.
        if(remaining_>0)--remaining_;next_=now+interval_;
    }
    void Cancel() { remaining_=0; }
private:
    int remaining_=0;
    uint32_t next_=0,ready_=0,interval_=100;
    bool cooling_=false;
};
