#pragma once
#include <algorithm>
#include <cmath>

// One swing has one delayed contact, even if the player holds attack or changes view during windup.
class HeroAttackTimeline {
public:
    bool Begin(double now,float delay,float interval,float attackSpeed) {
        if(now<ready_ || pending_)return false;
        float speed=std::isfinite(attackSpeed) ? std::clamp(attackSpeed,.2f,5.f) : 1.f;
        contact_=now+std::min(delay,interval)/speed;ready_=now+interval/speed;pending_=true;return true;
    }
    bool Contact(double now) {if(!pending_ || now<contact_)return false;pending_=false;return true;}
    bool Busy(double now) const {return pending_ || now<ready_;}
    void Cancel() {pending_=false;} // Interrupting a swing cannot reset its recovery and increase DPS.
    void Reset() {pending_=false;ready_=contact_=0;}
private:
    double contact_=0,ready_=0;
    bool pending_=false;
};
