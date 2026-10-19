#pragma once
#include "../config/GrenadeSettings.hpp"
#include "../geometry/RayBounds.hpp"
#include <array>
#include <vector>

using GrenadePoint=std::array<float,3>;

// Fixed substeps give identical flights on fast and slow renderers; collision is supplied by the native adapter.
class GrenadeProjectile {
public:
    GrenadePoint position{},velocity{};
    GrenadeSettings settings;
    double age=0;
    bool bounced=false;
    template<class Ground> void Advance(double elapsed,const std::vector<Bounds3>& obstacles,Ground ground) {
        double target=std::min(elapsed,double(settings.fuse));bounced=false;
        while(age+1e-8<target) {
            float dt=float(std::min(1.0/120,target-age));age+=dt;
            GrenadePoint next=position;velocity[2]-=settings.gravity*dt;
            for(int i=0;i<3;++i)next[i]+=velocity[i]*dt;
            // Sweep a small hull so a thin wall cannot be crossed between simulation frames.
            float fraction=1;int axis=-1;float sign=0;
            for(const auto& source:obstacles) {
                auto box=source;for(int i=0;i<3;++i){box.minimum[i]-=4;box.maximum[i]+=4;}
                GrenadePoint delta{};for(int i=0;i<3;++i)delta[i]=next[i]-position[i];
                float entry=0;if(!IntersectBounds(box,position.data(),delta.data(),1,entry))continue;
                if(entry>=fraction)continue;
                // Ignore a volume already containing the origin; never teleport out through a remote face.
                int normal=-1;float side=0,best=1e9f;
                for(int i=0;i<3;++i)if(std::abs(delta[i])>1e-6f) {
                    float plane=delta[i]>0 ? box.minimum[i] : box.maximum[i];
                    float distance=std::abs(position[i]+delta[i]*entry-plane);
                    if(distance<best){best=distance;normal=i;side=delta[i]>0 ? -1.f : 1.f;}
                }
                if(entry<1e-6f)continue;
                fraction=entry;axis=normal;sign=side;
            }
            if(axis>=0) {
                for(int i=0;i<3;++i)next[i]=position[i]+(next[i]-position[i])*std::max(0.f,fraction-.001f);
                next[axis]+=sign*.1f;velocity[axis]*=-settings.bounce;bounced=true;
            }
            float floor=ground(next[0],next[1])+4;
            if(next[2]<floor) {
                bool impact=std::abs(velocity[2])>=50 || std::hypot(velocity[0],velocity[1])>=35;
                if(floor>position[2]+12) {next[0]=position[0];next[1]=position[1];velocity[0]*=-settings.bounce;velocity[1]*=-settings.bounce;}
                else {next[2]=floor;velocity[2]=std::abs(velocity[2])<50 ? 0 : std::abs(velocity[2])*settings.bounce;velocity[0]*=.8f;velocity[1]*=.8f;}
                bounced|=impact;
            }
            position=next;
        }
    }
    static float Falloff(float distance,float radius) {return radius>0 ? std::clamp(1-distance/radius,0.f,1.f) : 0;}
    // Turning away reduces flash exposure; distance and facing both matter, without applying HP damage.
    static float Exposure(const GrenadePoint& from,const GrenadePoint& eye,const GrenadePoint& direction,float radius) {
        float distance=0,dot=0;for(int i=0;i<3;++i){float v=from[i]-eye[i];distance+=v*v;dot+=v*direction[i];}
        distance=std::sqrt(distance);float facing=distance>1 ? std::max(0.f,dot/distance) : 1;
        return Falloff(distance,radius)*(.2f+.8f*facing);
    }
};
