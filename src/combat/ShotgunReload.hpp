#pragma once
#include <algorithm>
#include <cmath>

// Full-magazine timing sets a constant insertion rate; partial reloads finish proportionally sooner.
class ShotgunReload {
public:
    struct Pose { const char* name; int phase; float elapsed; };
    void Configure(int shells,int capacity,float fullSeconds,float start,float insert,float finish) {
        capacity=std::max(1,capacity);shells_=std::clamp(shells,1,capacity);
        start_=Duration(start);insert_=Duration(insert);finish_=Duration(finish);
        rate_=(start_+capacity*insert_+finish_)/std::max(.1f,fullSeconds);
        seconds_=(start_+shells_*insert_+finish_)/rate_;
        credited_=0;
    }
    float Rate() const { return rate_; }
    float Seconds() const { return seconds_; }
    int Completed(float elapsed) const {
        // A shell belongs to the magazine only after its whole insertion clip has completed.
        float inserted=(std::clamp(elapsed,0.f,seconds_)*rate_-start_)/insert_;
        return std::clamp(int(std::floor(inserted+1e-5f)),0,shells_);
    }
    int LoadCompleted(float elapsed,int capacity,int& magazine,int& reserve) {
        int complete=Completed(elapsed),due=std::max(0,complete-credited_);credited_=std::max(credited_,complete);
        // Credit each completed insertion once, bounded by current space and actual reserve rounds.
        int amount=std::min({due,std::max(0,capacity-magazine),std::max(0,reserve)});
        magazine+=amount;reserve-=amount;return amount;
    }
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
    int shells_=1,credited_=0;
    float seconds_=1,start_=.5f,insert_=.5f,finish_=.5f,rate_=1.5f;
};
