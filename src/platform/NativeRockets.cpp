#include "NativeRockets.hpp"
#include "NativeBlast.hpp"
#include <algorithm>

bool NativeRockets::Clock(){if(timer_)return true;timer_=wc3::CreateTimer();if(!timer_)return false;float duration=360000;wc3::TimerStart(timer_,&duration,FALSE,0);return true;}
float NativeRockets::Time() const {return timer_ ? wc3::Real(wc3::TimerGetElapsed(timer_)) : 0;}
void NativeRockets::Reset(){++generation_;timer_=0;charges_.clear();bursts_.clear();visuals_.clear();obstacles_.Reset();nextLaser_=0;laserVisible_=false;}
bool NativeRockets::Active(wc3::Handle actor) const {for(const auto& charge:charges_)if(charge.actor==actor)return true;return false;}
bool NativeRockets::Launch(wc3::Handle actor,const GrenadePoint& eye,const GrenadePoint& direction,const GrenadePoint& motion,const GameplaySettings& settings) {
    if(!settings.WeaponEnabled(WeaponSlots::RPG) || charges_.size()>=settings.rpg.maxActive || !wc3::GetUnitTypeId(actor) || !Clock())return false;
    Charge charge;charge.actor=actor;charge.type=wc3::GetUnitTypeId(actor);charge.born=Time();charge.damage=settings.damage[WeaponSlots::RPG];charge.friendly=settings.friendlyFirePercent;
    charge.flight.settings=settings.rpg;charge.flight.range=settings.FirearmRange(WeaponSlots::RPG);
    // Rockets hit visible building models even when Warcraft uses a pathing texture instead of a unit collision radius.
    auto boxes=obstacles_.Snapshot(actor,eye[0],eye[1],128);
    if(!charge.flight.Launch(eye,direction,motion,boxes,[](float x,float y){return wc3::AttackSurface(x,y,false);}))return false;
    charges_.push_back(charge);
    wc3::Log("RPG launched at=%.1f,%.1f,%.1f velocity=%.1f,%.1f,%.1f damage=%.1f radius=%.1f",charge.flight.position[0],charge.flight.position[1],charge.flight.position[2],
        charge.flight.velocity[0],charge.flight.velocity[1],charge.flight.velocity[2],charge.damage,settings.rpg.radius);return true;
}
bool NativeRockets::Tick(GameAudio& audio,wc3::Handle viewer,bool guide,const GrenadePoint& eye,const GrenadePoint& direction,float guideRange) {
    if(guide && !Clock())guide=false;
    if(!timer_){audio.SetLoop("hl_rpg_motor.wav",false);laserVisible_=false;return false;}
    float now=Time();unsigned generation=generation_;bool hit=false,powered=false;
    if(guide && now>=nextLaser_) {
        // The designator traces actual world obstacles at motor frequency, rather than selecting a unit through walls.
        auto boxes=obstacles_.Snapshot(viewer,eye[0],eye[1],guideRange+256);GrenadePoint delta{};
        for(int i=0;i<3;++i)delta[i]=direction[i]*guideRange;
        float fraction=1;laserVisible_=RocketProjectile::Sweep(eye,delta,boxes,[](float x,float y){return wc3::AttackSurface(x,y,false);},fraction);
        for(int i=0;i<3;++i)laser_[i]=eye[i]+delta[i]*std::max(0.f,fraction-.0001f);
        nextLaser_=now+.1f;
    }
    if(!guide){laserVisible_=false;nextLaser_=0;}
    std::vector<Charge> impacts;visuals_.clear();
    for(auto& charge:charges_) {
        auto& flight=charge.flight;float elapsed=std::max(0.f,now-charge.born),pending=std::max(0.f,elapsed-float(flight.age));
        float reach=flight.Reach(pending);
        auto boxes=obstacles_.Snapshot(charge.actor,flight.position[0],flight.position[1],reach);
        const GrenadePoint* spot=guide && laserVisible_ && charge.actor==viewer && !NativeBlast::Covered(flight.position,laser_,boxes) ? &laser_ : nullptr;
        double previousAge=flight.age;
        flight.Advance(elapsed,boxes,[](float x,float y){return wc3::AttackSurface(x,y,false);},spot);
        if(int(flight.age)!=int(previousAge))wc3::Trace("RPG flight age=%.2f at=%.1f,%.1f,%.1f speed=%.1f obstacles=%u guiding=%d",flight.age,flight.position[0],flight.position[1],flight.position[2],RocketProjectile::Length(flight.velocity),unsigned(boxes.size()),spot!=nullptr);
        if(flight.done){impacts.push_back(charge);continue;}
        bool motor=elapsed>=flight.settings.ignition && elapsed<flight.settings.ignition+flight.settings.boostSeconds;powered|=motor;
        if(elapsed>=flight.settings.ignition && (charge.trail.empty() || now-charge.trail.back().time>=.04f))charge.trail.push_back({now,flight.position});
        while(!charge.trail.empty() && now-charge.trail.front().time>flight.settings.trailSeconds)charge.trail.pop_front();
        RocketVisual visual;visual.position=flight.position;visual.direction=flight.heading;visual.scale=flight.settings.modelScale;visual.trailSize=flight.settings.trailSize;visual.powered=motor;
        for(const auto& sample:charge.trail)visual.trail.push_back(sample.position);visuals_.push_back(std::move(visual));
    }
    // Finish traversal before native damage triggers can unload the map or alter the projectile collection.
    charges_.erase(std::remove_if(charges_.begin(),charges_.end(),[](const Charge& c){return c.flight.done;}),charges_.end());
    audio.SetLoop("hl_rpg_motor.wav",powered && viewer,.45f);
    for(const auto& charge:impacts) {
        if(generation_!=generation)return hit;
        auto from=charge.flight.position;from[2]+=4;
        wc3::Log("RPG exploded at=%.1f,%.1f,%.1f age=%.3f travelled=%.1f",from[0],from[1],from[2],charge.flight.age,charge.flight.travelled);
        audio.Play("hl_rpg_explode.wav",1);bursts_.push_back({from,now,charge.flight.settings.radius*.3f});
        auto effect=wc3::Effect("Objects\\Spawnmodels\\Other\\NeutralBuildingExplosion\\NeutralBuildingExplosion.mdl",from[0],from[1]);if(effect)wc3::DestroyEffect(effect);
        auto boxes=obstacles_.Snapshot(0,from[0],from[1],charge.flight.settings.radius+256);
        hit=NativeBlast::Apply(charge.actor,charge.type,from,charge.damage,charge.flight.settings.radius,charge.friendly,boxes,[&]{return generation_==generation;},"RPG") || hit;
    }
    if(generation_!=generation)return hit;
    bursts_.erase(std::remove_if(bursts_.begin(),bursts_.end(),[&](const Burst& b){return now-b.born>=.65f;}),bursts_.end());
    for(const auto& burst:bursts_){RocketVisual visual;visual.position=burst.position;visual.explosion=true;visual.age=now-burst.born;visual.scale=burst.size;visuals_.push_back(visual);}
    return hit;
}
