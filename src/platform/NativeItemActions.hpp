#pragma once
#include "SpellAim.hpp"

// Own explicit inventory orders through native approach/channeling; charges/cooldowns stay with Warcraft.
class NativeItemActions {
public:
    enum class Result {Rejected,Issued,Requested,TargetRequired};
    void Configure(uintptr_t base) {base_=base;}
    void Reset() {unit_=0;order_=0;}
    void Cancel();
    void Update();
    bool Active() const {return order_!=0;}
    Result Use(wc3::Handle unit,int slot,wc3::Handle expected,const SpellAim* aim,bool self=false);
    bool Drop(wc3::Handle unit,int slot,wc3::Handle expected,float yaw,float distance);
    bool Move(wc3::Handle unit,int slot,wc3::Handle expected,int destination);
private:
    bool Valid(wc3::Handle unit,int slot,wc3::Handle expected) const;
    void Begin(wc3::Handle unit,int order) {unit_=unit;order_=order;requested_=GetTickCount();}
    uintptr_t base_=0;
    wc3::Handle unit_=0;
    int order_=0;
    DWORD requested_=0;
};
