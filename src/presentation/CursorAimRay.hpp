#pragma once
#include <array>
#include <cmath>

namespace CursorAimRay {
inline std::array<float,3> Direction(float yaw,float pitch,float verticalFov,float x,float y,float width,float height) {
    constexpr float radians=.01745329252f;
    float cy=std::cos(yaw*radians),sy=std::sin(yaw*radians),cp=std::cos(pitch*radians),sp=std::sin(pitch*radians);
    std::array<float,3> result={cp*cy,cp*sy,sp};if(width<=0 || height<=0)return result;
    // Inverse of Warcraft's world-to-screen basis: screen right points toward negative world Y at yaw zero.
    float tangent=std::tan(verticalFov*radians*.5f),right=(2*x/width-1)*tangent*width/height,up=(1-2*y/height)*tangent;
    result[0]+=sy*right-sp*cy*up;result[1]+=-cy*right-sp*sy*up;result[2]+=cp*up;
    float length=std::sqrt(result[0]*result[0]+result[1]*result[1]+result[2]*result[2]);
    for(auto& value:result)value/=length;return result;
}
}
