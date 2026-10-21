#pragma once
#include "WeaponSlots.hpp"
#include "../config/HalfLifeSettings.hpp"
#include <algorithm>

// Charge/channel state owns spending and cadence; native contacts and audio consume its events.
class HalfLifeEnergy {
public:
    struct Event {bool shot=false,started=false,stopped=false,charged=false;float charge=0;};
    bool Active() const {return charging_ || channel_;}
    void Cancel() {charging_=channel_=false;}
    Event Step(int slot,double now,bool pressed,bool held,bool secondary,int& cells,float interval,const HalfLifeSettings& settings) {
        Event event;
        if(slot==WeaponSlots::Tau) {
            if(charging_) {
                double elapsed=std::max(0.0,now-start_);
                // Account elapsed charging even on the release frame; a hitch cannot grant free charge.
                int due=int(std::min(double(settings.tauChargeSeconds),elapsed)/settings.tauDrainSeconds);
                int spend=std::min(cells,std::max(0,due-drained_));cells-=spend;drained_+=spend;
                // Releasing discharges once, including when charging has just consumed the final cell.
                if(!secondary || cells==0) {
                    event.shot=event.charged=event.stopped=true;
                    float funded=std::min(float(elapsed),(drained_+1)*settings.tauDrainSeconds);
                    event.charge=std::clamp(funded/settings.tauChargeSeconds,0.f,1.f);
                    charging_=false;next_=now+interval;
                }
            } else if(now>=next_ && secondary && cells>0) {
                --cells;start_=now;drained_=0;charging_=true;event.started=true;
            } else if(now>=next_ && pressed && cells>=settings.tauPrimaryCost) {
                cells-=settings.tauPrimaryCost;event.shot=true;next_=now+interval;
            }
        } else if(slot==WeaponSlots::Gluon) {
            if(!held || cells<=0) {event.stopped=channel_;channel_=false;}
            else if(now>=next_) {event.started=!channel_;channel_=true;--cells;event.shot=true;next_=now+interval;}
        }
        return event;
    }
private:
    bool charging_=false,channel_=false;
    double start_=0,next_=0;
    int drained_=0;
};
