#pragma once
#include <cstdint>

// Start at the first presented frame, so a slow world draw cannot swallow a short accepted-shot flash.
class MuzzleFlashPulse {
public:
    float Sample(unsigned serial,bool allowed,uint32_t now,unsigned milliseconds) {
        if(!allowed){seen_=serial;active_=false;return 0;}
        if(serial!=seen_){seen_=serial;start_=now;active_=true;}
        if(!active_ || !milliseconds)return 0;
        uint32_t elapsed=now-start_;
        if(elapsed>=milliseconds){active_=false;return 0;}
        return 1-float(elapsed)/milliseconds;
    }
private:
    unsigned seen_=0;
    uint32_t start_=0;
    bool active_=false;
};
