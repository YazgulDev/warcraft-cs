#pragma once
#include <algorithm>
#include <cmath>

// One fitted timeline owns start, one insertion per missing shell, and the closing action.
class ShotgunReload {
public:
    struct Pose { const char* name; int phase; float elapsed; };
    void Configure(int shells,float seconds,float start,float insert,float finish) {
        shells_=std::max(1,shells);seconds_=std::max(.1f,seconds);
        start_=Duration(start);insert_=Duration(insert);finish_=Duration(finish);
        rate_=(start_+shells_*insert_+finish_)/seconds_;
    }
    float Rate() const { return rate_; }
    Pose At(float elapsed) const {
        float time=std::clamp(elapsed,0.f,seconds_)*rate_;
        if(time<start_)return {"start_reload",0,time/rate_};
        time-=start_;
        if(time<shells_*insert_) {
            int shell=std::min(int(time/insert_),shells_-1);
            return {"insert",shell+1,(time-shell*insert_)/rate_};
        }
        return {"after_reload",shells_+1,std::min(time-shells_*insert_,finish_)/rate_};
    }
private:
    // Missing optional metadata must not restart an insertion every frame.
    static float Duration(float value) { return std::isfinite(value) && value>0 ? value : .5f; }
    int shells_=1;
    float seconds_=1,start_=.5f,insert_=.5f,finish_=.5f,rate_=1.5f;
};
