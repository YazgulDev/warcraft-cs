#pragma once
#include "ViewmodelPose.hpp"
#include "WorldLabelView.hpp"
#include <array>
#include <cmath>

namespace ViewmodelAnchor {
inline std::array<float,3> World(const std::array<float,3>& anchor,const ViewmodelPose& pose,float projectionScale,const WorldLabelView& view,float heldAspect=0,float nearPlane=0) {
    // Held weapons and native scenery use different projections. Preserve the attachment's screen position,
    // including decorative roll/bob, while moving it into the native depth-tested world beam pass.
    constexpr float radians=.01745329252f;
    float roll=pose.roll*radians,c=std::cos(roll),s=std::sin(roll);
    float x=anchor[0]*c-anchor[1]*s+pose.x,y=anchor[0]*s+anchor[1]*c+pose.y,z=anchor[2]+pose.z;
    float ratio=std::tan(view.verticalFov*radians*.5f)/projectionScale;x*=ratio;y*=ratio;
    // Warcraft's native aspect can differ from the full-window weapon projection, especially on wide monitors.
    if(heldAspect>0)x*=view.aspect/heldAspect;
    // Keep a custom near plane from clipping the nozzle away; scaling along its eye ray preserves the exact pixel.
    if(nearPlane>0 && -z>0 && -z<=nearPlane){float scale=(nearPlane+.01f)/(-z);x*=scale;y*=scale;z*=scale;}
    float cy=std::cos(view.yaw*radians),sy=std::sin(view.yaw*radians),cp=std::cos(view.pitch*radians),sp=std::sin(view.pitch*radians);
    return {view.eye[0]+sy*x-cy*sp*y-cy*cp*z,view.eye[1]-cy*x-sy*sp*y-sy*cp*z,view.eye[2]+cp*y-sp*z};
}
}
