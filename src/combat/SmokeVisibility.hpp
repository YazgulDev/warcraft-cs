#pragma once
#include "GrenadeVisual.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

// Optical depth is shared by visual density and all-team AI targeting; bullets retain their physical ray.
namespace SmokeVisibility {
inline GrenadePoint Center(const GrenadeVisual& cloud) {
    auto center=cloud.position;center[2]+=cloud.radius*.5f;return center;
}
inline float Length(const GrenadeVisual& cloud,const GrenadePoint& from,const GrenadePoint& to) {
    if(!cloud.smoke || cloud.radius<=0 || cloud.opacity<=0)return 0;
    auto center=Center(cloud);float a=0,b=0,c=-cloud.radius*cloud.radius;
    for(int i=0;i<3;++i){float d=to[i]-from[i],p=from[i]-center[i];a+=d*d;b+=p*d;c+=p*p;}
    if(a<.000001f)return 0;
    float discriminant=b*b-a*c;if(discriminant<=0)return 0;
    float root=std::sqrt(discriminant);
    return std::max(0.f,std::min(1.f,(-b+root)/a)-std::max(0.f,(-b-root)/a))*std::sqrt(a);
}
inline float Density(const GrenadeVisual& cloud) {return 4.605170186f*cloud.opacity/std::max(16.f,cloud.visibilityDistance);}
inline float Transmission(const std::vector<GrenadeVisual>& clouds,const GrenadePoint& from,const GrenadePoint& to,bool ai=false) {
    float optical=0;for(const auto& cloud:clouds)if(!ai || cloud.blockAI)optical+=Length(cloud,from,to)*Density(cloud);
    return std::exp(-optical);
}
inline bool Visible(const std::vector<GrenadeVisual>& clouds,const GrenadePoint& from,const GrenadePoint& to,bool ai=false) {
    return Transmission(clouds,from,to,ai)>.1f;
}
}
