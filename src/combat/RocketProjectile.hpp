#pragma once
#include "GrenadeProjectile.hpp"
#include "../config/RpgSettings.hpp"

// Portable, swept rocket flight: short gravity launch, motor acceleration and optional laser steering.
class RocketProjectile {
public:
    GrenadePoint position{},velocity{},heading{};
    RpgSettings settings;
    double age=0,nextThink=.5;
    float travelled=0,range=30000;
    bool done=false;
    static float Length(const GrenadePoint& value) {return std::sqrt(value[0]*value[0]+value[1]*value[1]+value[2]*value[2]);}
    static GrenadePoint Unit(GrenadePoint value) {float n=Length(value);if(n>1e-6f)for(auto& v:value)v/=n;return value;}
    template<class Ground> static bool Sweep(const GrenadePoint& origin,const GrenadePoint& delta,
        const std::vector<Bounds3>& obstacles,Ground ground,float& fraction) {
        fraction=1;bool hit=false;
        for(auto box:obstacles) {
            for(int i=0;i<3;++i){box.minimum[i]-=2;box.maximum[i]+=2;}
            float entry=0;if(IntersectBounds(box,origin.data(),delta.data(),fraction,entry)){fraction=entry;hit=true;}
        }
        int steps=std::max(1,int(std::ceil(Length(delta)/4)));
        for(int step=1;step<=steps;++step) {
            float t=float(step)/steps;if(t>fraction)break;
            float floor=ground(origin[0]+delta[0]*t,origin[1]+delta[1]*t);
            if(!std::isfinite(floor) || floor+2>=origin[2]+delta[2]*t) {
                float lo=float(step-1)/steps,hi=t;
                // Refine the first terrain contact, so high-speed shots cannot skip a narrow raised lip.
                for(int n=0;n<8;++n){float mid=(lo+hi)*.5f;if(ground(origin[0]+delta[0]*mid,origin[1]+delta[1]*mid)+2>=origin[2]+delta[2]*mid)hi=mid;else lo=mid;}
                fraction=lo;hit=true;break;
            }
        }
        return hit;
    }
    float Reach(float pending) const {
        // A custom launch can exceed its later motor cap; slow-frame queries must cover both phases.
        return std::min(50000.f,std::max(Length(velocity),settings.maximumSpeed)*pending+.5f*settings.gravity*pending*pending+256);
    }
    template<class Ground> bool Launch(const GrenadePoint& eye,const GrenadePoint& aim,const GrenadePoint& motion,
        const std::vector<Bounds3>& obstacles,Ground ground) {
        for(int i=0;i<3;++i)if(!std::isfinite(eye[i]) || !std::isfinite(aim[i]) || !std::isfinite(motion[i]))return false;
        if(Length(aim)<1e-6f)return false;heading=Unit(aim);position=eye;age=travelled=0;done=false;nextThink=settings.ignition+.1;
        float yaw=std::atan2(heading[1],heading[0]),pitch=std::atan2(heading[2],std::hypot(heading[0],heading[1]));
        GrenadePoint right={std::sin(yaw),-std::cos(yaw),0},up={-std::cos(yaw)*std::sin(pitch),-std::sin(yaw)*std::sin(pitch),std::cos(pitch)},offset{};
        float inherited=0;for(int i=0;i<3;++i)inherited+=motion[i]*heading[i];
        constexpr float lift=.5235987756f;
        GrenadePoint launch={std::cos(yaw)*std::cos(pitch+lift),std::sin(yaw)*std::cos(pitch+lift),std::sin(pitch+lift)};
        for(int i=0;i<3;++i){offset[i]=heading[i]*24+right[i]*12-up[i]*12;velocity[i]=launch[i]*settings.launchSpeed+heading[i]*inherited*settings.inheritedVelocity;}
        float fraction=1;done=Sweep(eye,offset,obstacles,ground,fraction);
        for(int i=0;i<3;++i)position[i]+=offset[i]*std::max(0.f,fraction-.001f);
        return true;
    }
    template<class Ground> void Advance(double elapsed,const std::vector<Bounds3>& obstacles,Ground ground,const GrenadePoint* spot=nullptr) {
        double target=std::min(elapsed,double(settings.lifetime));
        while(!done && age+1e-8<target) {
            // Steering uses HL-style tenth-second motor pulses, independent of renderer frame rate.
            if(age+1e-7>=nextThink) {
                if(spot){GrenadePoint to{};float dot=0;for(int i=0;i<3;++i)to[i]=(*spot)[i]-position[i];to=Unit(to);
                    for(int i=0;i<3;++i)dot+=to[i]*heading[i];if(dot>0 && Length(to)>.5f)heading=to;}
                float speed=Length(velocity);bool boost=age<settings.ignition+settings.boostSeconds;
                for(int i=0;i<3;++i)velocity[i]=velocity[i]*.2f+heading[i]*(speed*(boost ? .8f : .798f)+(boost ? settings.acceleration*.1f : 0));
                speed=Length(velocity);if(speed>settings.maximumSpeed)for(auto& v:velocity)v*=settings.maximumSpeed/speed;
                if(!boost && speed<settings.maximumSpeed*.75f)done=true;
                nextThink+=.1;if(done)break;
            }
            double boundary=age<settings.ignition ? settings.ignition : nextThink;
            float dt=float(std::min({1.0/120,target-age,std::max(1e-8,boundary-age)}));
            if(age<settings.ignition)velocity[2]-=settings.gravity*dt;
            GrenadePoint delta{};for(int i=0;i<3;++i)delta[i]=velocity[i]*dt;
            float distance=Length(delta),fraction=1;bool impact=Sweep(position,delta,obstacles,ground,fraction);
            if(travelled+distance*fraction>=range){fraction=std::min(fraction,std::max(0.f,range-travelled)/std::max(distance,.001f));impact=true;}
            for(int i=0;i<3;++i)position[i]+=delta[i]*std::max(0.f,fraction-.001f*(impact ? 1.f : 0.f));
            travelled+=distance*fraction;age+=dt;done=impact;
        }
        if(age+1e-6>=settings.lifetime)done=true;
    }
};
