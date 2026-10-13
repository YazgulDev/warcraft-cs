#pragma once
#include <algorithm>
#include <cmath>

// Visual backswing is independent of attack cooldown; movement resumes when the authored clip finishes.
class HeroAttackPose {
public:
    void Begin(double now,float seconds,float rate,float contact,float recovery) {
        start_=now;rate_=std::isfinite(rate) ? std::max(0.f,rate) : 1;
        seconds_=std::max(0.f,seconds);
        end_=now+std::max(contact,std::min(recovery,rate_>0 ? seconds_/rate_ : recovery));
    }
    bool Active(double now) const {return now<end_;}
    float Elapsed(double now) const {return std::clamp(float(now-start_)*rate_,0.f,std::max(0.f,seconds_-.001f));}
    float Rate() const {return rate_;}
    void Reset() {start_=end_=0;rate_=1;seconds_=0;}
private:
    double start_=0,end_=0;
    float rate_=1,seconds_=0;
};
