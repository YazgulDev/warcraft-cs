#pragma once
#include "WarcraftApi.hpp"
#include "../config/PortalSettings.hpp"
#include <functional>

// Native waygate destinations are map-owned. This adapter supplies the proximity interaction missing from direct XY movement.
class NativePortals {
public:
    void Configure(uintptr_t module);
    void Reset();
    bool Observe(wc3::Handle actor,float tolerance);
    void Commit(wc3::Handle actor);
    bool Travel(wc3::Handle actor,float feet,const PortalSettings& settings,bool canMove,const std::function<bool()>& valid);
private:
    using Active=BOOL (__cdecl*)(wc3::Handle);
    using Coordinate=wc3::Bits (__cdecl*)(wc3::Handle);
    Active active_=nullptr;Coordinate destinationX_=nullptr,destinationY_=nullptr;
    wc3::Handle actor_=0,timer_=0;uintptr_t actorObject_=0;
    float nextScan_=0,nextTravel_=0;
    bool leaveGate_=false;
    bool tracked_=false;float lastX_=0,lastY_=0;
    bool Select(wc3::Handle actor);
    unsigned generation_=0;
};
