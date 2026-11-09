#pragma once
#include "MovementObstacles.hpp"
#include "NativeBlast.hpp"
#include "../combat/UnitStatus.hpp"
#include "../combat/BlastJumpEvent.hpp"
#include "../movement/MovementPhysics.hpp"
#include "../movement/MovementImpulse.hpp"

// Native eligibility/cover stays separate from portable velocity and normal swept collision.
class NativeMovementImpulse {
public:
    void Configure(uintptr_t base){base_=base;obstacles_.Configure(base);}
    void Reset(){obstacles_.Reset();}
    bool Blast(wc3::Handle unit,MovementPhysics& motion,const BlastJumpEvent& event) {
        if(!event.settings.enabled || event.settings.speed<=0 || !Allowed(unit))return false;
        GrenadePoint center={wc3::Real(wc3::GetUnitX(unit)),wc3::Real(wc3::GetUnitY(unit)),
            motion.FeetZ()+wc3::Real(wc3::GetUnitFlyHeight(unit))+motion.BodyHeight()*.5f};
        auto delta=MovementImpulse::Explosion(event.position,center,event.radius,event.settings.speed*MovementPhysics::worldScale);
        if(std::hypot(std::hypot(delta[0],delta[1]),delta[2])<.001f)return false;
        auto boxes=obstacles_.Snapshot(unit,event.position[0],event.position[1],event.radius+256,true);
        if(NativeBlast::Covered(event.position,center,boxes))return false;
        return Commit(unit,motion,delta,event.settings.maximumUpSpeed,event.label);
    }
    bool Recoil(wc3::Handle unit,MovementPhysics& motion,const GrenadePoint& direction,float speed,float maximumUpSpeed) {
        if(speed<=0 || !Allowed(unit))return false;
        GrenadePoint delta{};
        // Use the accepted physical shot ray; looking down makes recoil point upward.
        for(int i=0;i<3;++i)delta[i]=-direction[i]*speed*MovementPhysics::worldScale;
        return Commit(unit,motion,delta,maximumUpSpeed,"Tau");
    }
private:
    bool Allowed(wc3::Handle unit) const {
        if(!base_ || !unit || !wc3::GetUnitTypeId(unit) || wc3::Real(wc3::GetUnitState(unit,0))<=.405f)return false;
        auto state=UnitStatus::Read(unit,base_);
        // Impulse jumps cannot escape cargo, native stuns/roots or hidden scripted control.
        return !state.incapacitated && !state.immobilized && !state.contained && !state.hidden;
    }
    bool Commit(wc3::Handle unit,MovementPhysics& motion,const GrenadePoint& delta,float maximumUpSpeed,const char* label) {
        bool accepted=motion.AddImpulse(delta[0],delta[1],delta[2],maximumUpSpeed*MovementPhysics::worldScale);
        if(accepted)wc3::Log("movement impulse source=%s actor=%08X delta=%.1f,%.1f,%.1f velocity=%.1f,%.1f,%.1f",label,unit,
            delta[0],delta[1],delta[2],motion.VelocityX(),motion.VelocityY(),motion.VerticalVelocity());
        return accepted;
    }
    uintptr_t base_=0;
    MovementObstacles obstacles_;
};
