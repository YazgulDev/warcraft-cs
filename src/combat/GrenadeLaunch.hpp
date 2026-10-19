#pragma once
#include "GrenadeProjectile.hpp"

// Portable CS-style release geometry; speed/gravity are already in Warcraft world units.
struct GrenadeLaunch {
    static bool Prepare(GrenadeProjectile& flight,const GrenadePoint& eye,const GrenadePoint& aim,const GrenadePoint& motion) {
        for(int i=0;i<3;++i)if(!std::isfinite(eye[i]) || !std::isfinite(aim[i]) || !std::isfinite(motion[i]))return false;
        float horizontal=std::hypot(aim[0],aim[1]);
        if(std::hypot(horizontal,aim[2])<1e-6f)return false;
        constexpr float radians=.01745329252f;
        float pitch=std::atan2(aim[2],horizontal)/radians;
        // CS remaps view pitch around an upward ten-degree level throw, then caps angle-dependent speed.
        float releasePitch=10+pitch*(pitch>0 ? 80.f/90 : 100.f/90);
        float speed=flight.settings.throwSpeed*std::clamp((90+releasePitch)/125,0.f,1.f);
        GrenadePoint forward={std::cos(releasePitch*radians)*(horizontal>1e-6f ? aim[0]/horizontal : 1),
            std::cos(releasePitch*radians)*(horizontal>1e-6f ? aim[1]/horizontal : 0),std::sin(releasePitch*radians)};
        for(int i=0;i<3;++i) {
            // Sixteen CS units forward at this project's established 1.5 world scale.
            flight.position[i]=eye[i]+forward[i]*24;
            flight.velocity[i]=forward[i]*speed+motion[i]*flight.settings.inheritedVelocity;
        }
        return true;
    }
    template<class Ground> static void ClipRelease(GrenadeProjectile& flight,const GrenadePoint& eye,
        const std::vector<Bounds3>& obstacles,Ground ground) {
        GrenadePoint delta{};for(int i=0;i<3;++i)delta[i]=flight.position[i]-eye[i];
        float fraction=1;
        // The forward release must not place a grenade through a nearby wall or raised terrain.
        for(auto box:obstacles) {
            for(int i=0;i<3;++i){box.minimum[i]-=4;box.maximum[i]+=4;}
            float entry=0;if(IntersectBounds(box,eye.data(),delta.data(),1,entry))fraction=std::min(fraction,std::max(0.f,entry-.001f));
        }
        for(int i=1;i<=8;++i) {
            float t=i/8.f;
            if(t>fraction)break;
            float floor=ground(eye[0]+delta[0]*t,eye[1]+delta[1]*t);
            if(!std::isfinite(floor) || floor+4>eye[2]+delta[2]*t){fraction=(i-1)/8.f;break;}
        }
        for(int i=0;i<3;++i)flight.position[i]=eye[i]+delta[i]*fraction;
    }
};
