#include "NativeGrenades.hpp"
#include "../combat/CombatDamage.hpp"
#include "../combat/GrenadeLaunch.hpp"
#include <algorithm>
#include <cmath>

namespace {
float Distance(const GrenadePoint& a,const GrenadePoint& b) {float n=0;for(int i=0;i<3;++i)n+=(a[i]-b[i])*(a[i]-b[i]);return std::sqrt(n);}
bool Contains(const Bounds3& box,const GrenadePoint& p) {for(int i=0;i<3;++i)if(p[i]<box.minimum[i] || p[i]>box.maximum[i])return false;return true;}
}
float NativeGrenades::Time() const {return timer_ ? wc3::Real(wc3::TimerGetElapsed(timer_)) : 0;}
void NativeGrenades::Reset() {
    // Map unload already owns all timer/unit handles. Never unpause a handle recycled by the next map.
    ++generation_;timer_=0;charges_.clear();stuns_.clear();visuals_.clear();obstacles_.Reset();flashStart_=flashDuration_=flashOpacity_=0;
}
bool NativeGrenades::Throw(wc3::Handle actor,int slot,const GrenadePoint& eye,const GrenadePoint& direction,const GrenadePoint& motion,const GameplaySettings& settings) {
    if(!settings.grenades.enabled || !WeaponSlots::Grenade(slot) || charges_.size()>=settings.grenades.maxActive || !wc3::GetUnitTypeId(actor))return false;
    if(!timer_) {timer_=wc3::CreateTimer();if(!timer_)return false;float duration=360000;wc3::TimerStart(timer_,&duration,FALSE,0);}
    Charge charge;charge.slot=slot;charge.actor=actor;charge.type=wc3::GetUnitTypeId(actor);charge.born=Time();charge.damage=settings.damage[slot];charge.friendly=settings.friendlyFirePercent;
    charge.flight.settings=settings.grenades;
    if(slot!=WeaponSlots::HE)charge.flight.settings.gravity*=settings.grenades.lightGravityScale;
    if(!GrenadeLaunch::Prepare(charge.flight,eye,direction,motion))return false;
    auto releaseObstacles=obstacles_.Snapshot(actor,eye[0],eye[1],64,true);
    GrenadeLaunch::ClipRelease(charge.flight,eye,releaseObstacles,[](float x,float y){return wc3::AttackSurface(x,y,false);});
    charges_.push_back(charge);
    wc3::Log("grenade launched slot=%d at=%.1f,%.1f,%.1f velocity=%.1f,%.1f,%.1f gravity=%.1f fuse=%.2f damage=%.1f",slot,
        charge.flight.position[0],charge.flight.position[1],charge.flight.position[2],charge.flight.velocity[0],charge.flight.velocity[1],charge.flight.velocity[2],
        charge.flight.settings.gravity,charge.flight.settings.fuse,charge.damage);return true;
}
bool NativeGrenades::Covered(const GrenadePoint& from,const GrenadePoint& to,const std::vector<Bounds3>& obstacles) const {
    GrenadePoint direction{};for(int i=0;i<3;++i)direction[i]=to[i]-from[i];
    for(const auto& box:obstacles) {
        // Endpoints inside their own units do not make the target or planter an artificial shield.
        if(Contains(box,from) || Contains(box,to))continue;
        float entry=0;if(IntersectBounds(box,from.data(),direction.data(),1,entry))return true;
    }
    float distance=Distance(from,to);int steps=std::max(1,int(distance/24));
    for(int i=1;i<steps;++i){float t=float(i)/steps;if(wc3::AttackSurface(from[0]+direction[0]*t,from[1]+direction[1]*t,false)>from[2]+direction[2]*t)return true;}
    return false;
}
bool NativeGrenades::Detonate(const Charge& charge,float now,GameAudio& audio,wc3::Handle viewer,const GrenadePoint& eye,const GrenadePoint& direction) {
    const auto& settings=charge.flight.settings;auto from=charge.flight.position;from[2]+=8;
    audio.Play(charge.slot==WeaponSlots::HE ? "hegrenade-1.wav" : charge.slot==WeaponSlots::Flashbang ? "flashbang-1.wav" : "sg_explode.wav",1);
    wc3::Log("grenade detonated slot=%d at=%.1f,%.1f,%.1f",charge.slot,from[0],from[1],from[2]);
    if(charge.slot==WeaponSlots::Smoke)return false;
    if(charge.slot==WeaponSlots::HE) {
        auto effect=wc3::Effect("Objects\\Spawnmodels\\Other\\NeutralBuildingExplosion\\NeutralBuildingExplosion.mdl",from[0],from[1]);if(effect)wc3::DestroyEffect(effect);
    }
    float radius=charge.slot==WeaponSlots::HE ? settings.heRadius : settings.flashRadius;
    auto obstacles=obstacles_.Snapshot(0,from[0],from[1],radius+256,true);
    if(charge.slot==WeaponSlots::Flashbang && viewer && !Covered(from,eye,obstacles)) {
        float exposure=GrenadeProjectile::Exposure(from,eye,direction,radius);
        float duration=settings.flashSeconds*exposure;
        if(duration>0 && now+duration>flashStart_+flashDuration_){flashStart_=now;flashDuration_=duration;wc3::Log("grenade viewer flash exposure=%.3f seconds=%.3f",exposure,duration);}
    }
    if(wc3::GetUnitTypeId(charge.actor)!=charge.type)return false;
    unsigned generation=generation_;auto owner=wc3::GetOwningPlayer(charge.actor);
    auto group=wc3::CreateGroup();if(!group)return false;
    wc3::GroupEnumUnitsInRange(group,&from[0],&from[1],&radius,0);std::vector<wc3::Handle> targets;
    for(auto target=wc3::FirstOfGroup(group);target;target=wc3::FirstOfGroup(group)){wc3::GroupRemoveUnit(group,target);targets.push_back(target);}
    wc3::DestroyGroup(group);
    // Take an immutable victim list before native damage/death callbacks mutate the map; damage the thrower last.
    std::stable_sort(targets.begin(),targets.end(),[&](auto a,auto b){return a!=charge.actor && b==charge.actor;});bool hit=false;
    for(auto target:targets) {
        if(generation_!=generation)return hit;
        if(!wc3::GetUnitTypeId(target) || wc3::IsUnitHidden(target) || wc3::IsUnitLoaded(target))continue;
        float before=wc3::Real(wc3::GetUnitState(target,0));if(before<=.405f)continue;
        GrenadePoint point={wc3::Real(wc3::GetUnitX(target)),wc3::Real(wc3::GetUnitY(target)),0};point[2]=wc3::Ground(point[0],point[1])+wc3::Real(wc3::GetUnitFlyHeight(target))+32;
        float falloff=GrenadeProjectile::Falloff(Distance(from,point),radius);if(falloff<=0 || Covered(from,point,obstacles))continue;
        if(charge.slot==WeaponSlots::HE) {
            if(wc3::GetUnitTypeId(charge.actor)!=charge.type)break;
            float damage=CombatDamage::Amount(charge.damage*falloff,wc3::IsUnitAlly(target,owner)!=FALSE,false,before,charge.friendly);
            if(damage>0)wc3::UnitDamageTarget(charge.actor,target,&damage,FALSE,TRUE,0,4,0);
            if(generation_!=generation)return hit;
            float after=wc3::GetUnitTypeId(target) ? wc3::Real(wc3::GetUnitState(target,0)) : 0;hit|=after<before;
            wc3::Log("grenade HE target=%08X damage=%.1f hpBefore=%.1f hpAfter=%.1f",target,damage,before,after);
        } else if(target!=viewer && settings.flashUnitStunSeconds>0 && wc3::IsUnitEnemy(target,owner) && !wc3::IsUnitType(target,2)) {
            auto found=std::find_if(stuns_.begin(),stuns_.end(),[&](const Stun& item){return item.unit==target;});
            float until=now+settings.flashUnitStunSeconds*falloff;
            if(found!=stuns_.end())found->until=std::max(found->until,until);
            else if(!wc3::IsUnitPaused(target)) {wc3::PauseUnit(target,TRUE);stuns_.push_back({target,wc3::GetUnitTypeId(target),until});wc3::Log("grenade flash disoriented target=%08X seconds=%.2f",target,until-now);}
        }
    }
    // Gates are destructable widgets and retain their native quest immunity.
    if(charge.slot==WeaponSlots::HE && generation_==generation && wc3::GetUnitTypeId(charge.actor)==charge.type)
        for(auto target:wc3::NearbyDestructables(from[0],from[1],radius)) {
            if(!wc3::GetDestructableTypeId(target) || wc3::IsDestructableInvulnerable(target))continue;
            float before=wc3::Real(wc3::GetDestructableLife(target));if(before<=.405f)continue;
            GrenadePoint point={wc3::Real(wc3::GetDestructableX(target)),wc3::Real(wc3::GetDestructableY(target)),0};point[2]=wc3::Ground(point[0],point[1])+32;
            float damage=charge.damage*GrenadeProjectile::Falloff(Distance(from,point),radius);if(damage<=0 || Covered(from,point,obstacles))continue;
            wc3::UnitDamageTarget(charge.actor,target,&damage,FALSE,TRUE,0,4,0);
            if(generation_!=generation)return hit;
            float after=wc3::GetDestructableTypeId(target) ? wc3::Real(wc3::GetDestructableLife(target)) : 0;hit|=after<before;
            wc3::Log("grenade HE gate=%08X damage=%.1f hpBefore=%.1f hpAfter=%.1f",target,damage,before,after);
        }
    return hit;
}
bool NativeGrenades::Tick(GameAudio& audio,wc3::Handle viewer,const GrenadePoint& eye,const GrenadePoint& direction) {
    if(!timer_)return false;float now=Time();unsigned generation=generation_;
    // Only pauses acquired by this adapter are released; pre-paused campaign units are never leased.
    for(auto it=stuns_.begin();it!=stuns_.end();) {
        if(now>=it->until) {if(wc3::GetUnitTypeId(it->unit)==it->type && wc3::IsUnitPaused(it->unit))wc3::PauseUnit(it->unit,FALSE);wc3::Log("grenade flash released target=%08X",it->unit);it=stuns_.erase(it);}else ++it;
    }
    std::vector<Charge> events;visuals_.clear();
    for(auto& charge:charges_) {
        auto& flight=charge.flight;float elapsed=std::max(0.f,now-charge.born),effectAge=elapsed-flight.settings.fuse;
        if(!charge.detonated) {
            float pending=std::max(0.f,std::min(elapsed,flight.settings.fuse)-float(flight.age));
            float speed=std::sqrt(flight.velocity[0]*flight.velocity[0]+flight.velocity[1]*flight.velocity[1]+flight.velocity[2]*flight.velocity[2]);
            // Cover the whole pending flight even after a slow frame, rather than querying only its first quarter-second.
            float reach=std::min(20000.f,speed*pending+.5f*flight.settings.gravity*pending*pending+256);
            auto obstacles=obstacles_.Snapshot(charge.actor,flight.position[0],flight.position[1],reach,true);
            flight.Advance(elapsed,obstacles,[](float x,float y){return wc3::AttackSurface(x,y,false);});
            if(flight.bounced)audio.Play("grenade_hit1.wav",.3f);
            if(elapsed>=flight.settings.fuse){charge.detonated=true;events.push_back(charge);}
            else {
                bool resting=std::abs(flight.velocity[2])<1 && std::hypot(flight.velocity[0],flight.velocity[1])<10;
                // Only the visual prop spins; settling restores the retail model's flat resting pose.
                float yaw=std::atan2(flight.velocity[1],flight.velocity[0])*57.2957795f;
                visuals_.push_back({flight.position,charge.slot,5,1,elapsed,false,yaw,resting ? 0 : elapsed*flight.settings.worldSpin,flight.settings.worldModelScale});
            }
        }
        if(charge.detonated && charge.slot==WeaponSlots::Smoke) {
            float opacity=flight.settings.smokeOpacity*std::min(1.f,effectAge)*std::clamp((flight.settings.smokeSeconds-effectAge)/3,0.f,1.f);
            if(opacity>0)visuals_.push_back({flight.position,charge.slot,flight.settings.smokeRadius,opacity,effectAge,true});
        }
    }
    charges_.erase(std::remove_if(charges_.begin(),charges_.end(),[&](const Charge& c){return c.detonated && (c.slot!=WeaponSlots::Smoke || now-c.born>=c.flight.settings.fuse+c.flight.settings.smokeSeconds);}),charges_.end());
    bool hit=false;for(const auto& event:events){hit=Detonate(event,now,audio,viewer,eye,direction)||hit;if(generation_!=generation)return hit;}
    float remaining=flashStart_+flashDuration_-now;
    flashOpacity_=remaining>0 && flashDuration_>0 ? std::clamp(remaining/(flashDuration_*.6f),0.f,1.f) : 0;
    return hit;
}
