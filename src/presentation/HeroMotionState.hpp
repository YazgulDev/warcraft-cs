#pragma once
#include "../config/ThirdPersonSettings.hpp"

// Physical contacts drive presentation; playing a pose never creates a jump or moves the unit.
class HeroMotionState {
public:
    enum class Pose {Stand,Run,Takeoff,Air,Land,Walk};
    Pose Update(double now,bool grounded,float speed,const ThirdPersonSettings& settings,bool walking=false) {
        if(!grounded && previousGrounded_)airStart_=now;
        if(grounded && !previousGrounded_)landUntil_=now+settings.landingDuration;
        previousGrounded_=grounded;
        if(!grounded)return now-airStart_<settings.takeoffDuration ? Pose::Takeoff : Pose::Air;
        if(now<landUntil_)return Pose::Land;
        // Shift/crouch choose the authored slow gait; physical speed still drives playback, including slows.
        return speed>1 ? walking ? Pose::Walk : Pose::Run : Pose::Stand;
    }
    void Reset() {previousGrounded_=true;airStart_=landUntil_=0;}
private:
    bool previousGrounded_=true;
    double airStart_=0,landUntil_=0;
};
