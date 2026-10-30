#pragma once
#include "../config/CameraHeightSettings.hpp"
#include <algorithm>
#include <cmath>

// One bounded vertical correction removes abrupt ground steps without moving the player's collision body.
class CameraHeightMotion {
public:
    void Reset(){ready_=false;offset_=0;}
    float Offset() const{return offset_;}
    // Keep the presented eye inside the collision body when descending under a low ceiling or crouching.
    float EyeOffset(float eyeHeight,float bodyHeight) const {
        return std::clamp(offset_,-eyeHeight*.5f,std::max(0.f,bodyHeight-eyeHeight));
    }
    void Step(float x,float y,float feet,bool grounded,float dt,float stepHeight,const CameraHeightSettings& settings) {
        if(!std::isfinite(feet) || !std::isfinite(x) || !std::isfinite(y)){Reset();return;}
        bool relocated=ready_ && (std::hypot(x-x_,y-y_)>128 || std::abs(feet-feet_)>stepHeight);
        if(!ready_ || !settings.enabled || relocated)offset_=0;
        else if(std::isfinite(dt) && dt>0) {
            float decay=std::exp(-dt/settings.seconds);
            // Integrate a linear ground segment so the same slope behaves consistently across frame rates.
            float change=grounded && grounded_ ? feet-feet_ : 0;
            offset_=std::clamp(offset_*decay-change*settings.seconds/dt*(1-decay),-stepHeight,stepHeight);
        }
        // Airborne movement and crouching retain their physical trajectory; only an existing correction decays.
        x_=x;y_=y;feet_=feet;grounded_=grounded;ready_=true;
    }
private:
    float x_=0,y_=0,feet_=0,offset_=0;
    bool ready_=false,grounded_=false;
};
