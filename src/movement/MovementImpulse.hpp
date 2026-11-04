#pragma once
#include <array>
#include <algorithm>
#include <cmath>

// Geometry produces velocity only. The normal motor/swept collision still owns placement.
namespace MovementImpulse {
using Vector=std::array<float,3>;
inline Vector Explosion(const Vector& origin,const Vector& center,float radius,float speed) {
    Vector result{};float length=0;
    for(int i=0;i<3;++i){result[i]=center[i]-origin[i];length+=result[i]*result[i];}
    length=std::sqrt(length);
    if(!std::isfinite(length) || !std::isfinite(radius) || !std::isfinite(speed) || radius<=0 || speed<=0 || length>=radius)return {};
    // A mathematically coincident floor blast has no radial direction; choose upward, never NaN.
    if(length<.001f)return {0,0,speed};
    float gain=speed*(1-length/radius)/length;
    for(float& value:result)value*=gain;
    return result;
}
}
