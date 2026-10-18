#pragma once
#include <algorithm>

// Pulling the pin does not spend a charge or start its fuse; launch is a separate one-shot event.
class GrenadeThrow {
public:
    enum class Event {None,Pull,Throw,Launch,Finished};
    void Reset() {state_=0;released_=launched_=false;}
    bool Active() const {return state_!=0;}
    Event Step(double now,bool pressed,bool down,float pull,float delay,float recovery) {
        if(!state_){if(!pressed)return Event::None;state_=1;start_=now;return Event::Pull;}
        if(state_==1){released_|=!down;if(!released_ || now-start_<pull)return Event::None;state_=2;start_=now;return Event::Throw;}
        if(!launched_ && now-start_>=delay){launched_=true;return Event::Launch;}
        if(launched_ && now-start_>=std::max(delay,recovery)){Reset();return Event::Finished;}
        return Event::None;
    }
private:
    int state_=0;bool released_=false,launched_=false;double start_=0;
};
