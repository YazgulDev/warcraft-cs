#include "DroppedWeapons.hpp"
#include <cmath>

bool DroppedWeapons::Spawn(int slot,float x,float y,const WeaponDropSettings& settings) {
    if(!settings.enabled || drops_.size()>=settings.maximumDrops)return false;
    if(!timer_) {
        timer_=wc3::CreateTimer();if(!timer_)return false;
        float duration=10000000;wc3::TimerStart(timer_,&duration,FALSE,0);
    }
    // A stock chest works on every map and needs no redistributed weapon art or custom item rawcode.
    auto marker=wc3::Effect("Objects\\InventoryItems\\TreasureChest\\treasurechest.mdl",x,y);
    if(!marker)return false;
    drops_.push_back({slot,x,y,Time()+settings.lifetimeSeconds,marker});return true;
}
void DroppedWeapons::Tick(const WeaponDropSettings& settings,bool enabled) {
    // Warcraft simulation time freezes expiry during pause; disabling drops also removes existing markers.
    float now=Time();
    for(int i=int(drops_.size())-1;i>=0;--i)
        if(!enabled || !settings.enabled || drops_[i].expires<=now || unsigned(i)>=settings.maximumDrops)Take(i);
}
int DroppedWeapons::Nearest(wc3::Handle actor,float radius) const {
    if(!actor)return -1;
    float x=wc3::Real(wc3::GetUnitX(actor)),y=wc3::Real(wc3::GetUnitY(actor)),best=radius*radius;int nearest=-1;
    for(int i=0;i<int(drops_.size());++i) {
        float dx=x-drops_[i].x,dy=y-drops_[i].y,distance=dx*dx+dy*dy;
        if(distance<=best){best=distance;nearest=i;}
    }
    return nearest;
}
void DroppedWeapons::Take(int index) {
    wc3::DestroyEffect(drops_[index].marker);drops_.erase(drops_.begin()+index);
}
void DroppedWeapons::Reset() {
    // Map unload already destroys native effects/timers; never dereference their recycled handles.
    drops_.clear();timer_=0;
}
