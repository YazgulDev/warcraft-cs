#include "NativePortals.hpp"
#include "NativeRegistry.hpp"
#include "MovementWorldBounds.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

void NativePortals::Configure(uintptr_t module) {
    Reset();
    active_=reinterpret_cast<Active>(NativeRegistry::Resolve(module,"WaygateIsActive","(Hunit;)B"));
    destinationX_=reinterpret_cast<Coordinate>(NativeRegistry::Resolve(module,"WaygateGetDestinationX","(Hunit;)R"));
    destinationY_=reinterpret_cast<Coordinate>(NativeRegistry::Resolve(module,"WaygateGetDestinationY","(Hunit;)R"));
    wc3::Log("Portal adapter verified=%d",active_ && destinationX_ && destinationY_);
}
void NativePortals::Reset() {
    ++generation_;actor_=timer_=0;actorObject_=0;nextScan_=nextTravel_=0;leaveGate_=tracked_=false;
}
bool NativePortals::Select(wc3::Handle actor) {
    auto object=wc3::UnitObject(actor);if(!object)return false;
    if(actor_!=actor || actorObject_!=object){actor_=actor;actorObject_=object;leaveGate_=tracked_=false;nextTravel_=nextScan_=0;}
    return true;
}
bool NativePortals::Observe(wc3::Handle actor,float tolerance,float* distance) {
    if(distance)*distance=0;
    if(!Select(actor))return false;
    float x=wc3::Real(wc3::GetUnitX(actor)),y=wc3::Real(wc3::GetUnitY(actor));
    float displacement=tracked_ ? std::hypot(x-lastX_,y-lastY_) : 0;
    if(distance)*distance=displacement;
    bool relocated=tracked_ && displacement>std::max(32.f,tolerance);
    // Native/scripted region teleports can occur between motor ticks. They need the same camera/floor
    // rebase and arrival latch, without guessing their trigger destinations or cancelling native casts.
    if(relocated){leaveGate_=true;wc3::Log("Portal/native relocation actor=%08X from=%.1f,%.1f actual=%.1f,%.1f distance=%.1f tolerance=%.1f",actor,lastX_,lastY_,x,y,displacement,tolerance);}
    lastX_=x;lastY_=y;tracked_=true;return relocated;
}
void NativePortals::Commit(wc3::Handle actor) {
    if(!Select(actor))return;
    lastX_=wc3::Real(wc3::GetUnitX(actor));lastY_=wc3::Real(wc3::GetUnitY(actor));tracked_=true;
}
bool NativePortals::Travel(wc3::Handle actor,float feet,const PortalSettings& settings,bool canMove,const std::function<bool()>& valid) {
    if(!settings.enabled || !active_ || !destinationX_ || !destinationY_ || !actor || !canMove || !valid || !valid() ||
        !wc3::GetUnitTypeId(actor) || wc3::IsUnitType(actor,2) || wc3::Real(wc3::GetUnitState(actor,0))<=.405f)return false;
    if(!Select(actor))return false;auto object=actorObject_;
    if(!timer_){timer_=wc3::CreateTimer();if(!timer_)return false;float duration=360000;wc3::TimerStart(timer_,&duration,FALSE,0);}
    float now=wc3::Real(wc3::TimerGetElapsed(timer_));if(now<nextScan_)return false;nextScan_=now+.1f;
    float x=wc3::Real(wc3::GetUnitX(actor)),y=wc3::Real(wc3::GetUnitY(actor));
    float range=settings.radius+(leaveGate_ ? 32.f : 0.f);auto group=wc3::CreateGroup();if(!group)return false;
    wc3::GroupEnumUnitsInRange(group,&x,&y,&range,0);
    wc3::Handle chosen=0;float nearest=std::numeric_limits<float>::max();
    for(auto gate=wc3::FirstOfGroup(group);gate;gate=wc3::FirstOfGroup(group)) {
        wc3::GroupRemoveUnit(group,gate);
        if(gate==actor || !wc3::GetUnitTypeId(gate) || wc3::IsUnitHidden(gate) || wc3::IsUnitLoaded(gate) ||
            wc3::Real(wc3::GetUnitState(gate,0))<=.405f || !active_(gate))continue;
        float gx=wc3::Real(wc3::GetUnitX(gate)),gy=wc3::Real(wc3::GetUnitY(gate));
        float gz=wc3::Ground(gx,gy)+wc3::Real(wc3::GetUnitFlyHeight(gate));
        float distance=std::hypot(gx-x,gy-y);
        if(distance<=range && std::isfinite(gz) && std::abs(feet-gz)<=settings.heightTolerance &&
            (distance<nearest || (distance==nearest && gate<chosen))){chosen=gate;nearest=distance;}
    }
    wc3::DestroyGroup(group);
    // Arrival never immediately feeds the reverse portal: leave its footprint before rearming.
    if(leaveGate_){if(!chosen)leaveGate_=false;return false;}
    if(!chosen || now<nextTravel_ || nearest>settings.radius)return false;
    float destinationX=wc3::Real(destinationX_(chosen)),destinationY=wc3::Real(destinationY_(chosen));
    auto bounds=MovementWorldBounds::Read(0);
    if(!std::isfinite(destinationX) || !std::isfinite(destinationY) || destinationX<bounds.minX || destinationX>bounds.maxX ||
        destinationY<bounds.minY || destinationY>bounds.maxY || std::hypot(destinationX-x,destinationY-y)<1)return false;
    auto generation=generation_;
    if(!valid() || wc3::UnitObject(actor)!=object || !active_(chosen))return false;
    // Native placement retains pathing/region processing; it may synchronously run map callbacks.
    wc3::SetUnitPosition(actor,&destinationX,&destinationY);
    if(generation_!=generation || !valid() || wc3::UnitObject(actor)!=object)return false;
    float actualX=wc3::Real(wc3::GetUnitX(actor)),actualY=wc3::Real(wc3::GetUnitY(actor));
    if(std::hypot(actualX-x,actualY-y)<1)return false;
    leaveGate_=true;nextTravel_=now+settings.cooldownSeconds;
    lastX_=actualX;lastY_=actualY;tracked_=true;
    wc3::Log("Portal travel gate=%08X actor=%08X from=%.1f,%.1f destination=%.1f,%.1f actual=%.1f,%.1f",chosen,actor,x,y,destinationX,destinationY,actualX,actualY);
    return true;
}
