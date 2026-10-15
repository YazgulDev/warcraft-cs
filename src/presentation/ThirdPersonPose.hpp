#pragma once
#include <array>
#include <cmath>

// A centered boom keeps the reticle and hero facing on the same horizontal bearing.
struct ThirdPersonPose {
    std::array<float,3> pivot{}, direction{}, eye{};
    static ThirdPersonPose Calculate(float x,float y,float z,float yaw,float pitch,float height,float distance) {
        constexpr float radians=.01745329252f;
        float a=yaw*radians,b=pitch*radians;
        ThirdPersonPose pose;
        pose.pivot={x,y,z+height};
        pose.direction={std::cos(b)*std::cos(a),std::cos(b)*std::sin(a),std::sin(b)};
        pose.Place(distance);return pose;
    }
    void Place(float distance) {
        for(int i=0;i<3;++i)eye[i]=pivot[i]-direction[i]*distance;
    }
};
