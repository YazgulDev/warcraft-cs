#pragma once
#include "WorldLabelView.hpp"
#include <cmath>

namespace WorldLabelVisibility {
inline bool Visible(const WorldLabelView& view, const float* position) {
    if (!view.active) return true;
    constexpr float radians=0.01745329252f;
    float x=position[0]-view.eye[0], y=position[1]-view.eye[1], z=position[2]-view.eye[2];
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
    if (x*x+y*y+z*z>view.maximumDistance*view.maximumDistance) return false;
    float cy=std::cos(view.yaw*radians), sy=std::sin(view.yaw*radians);
    float cp=std::cos(view.pitch*radians), sp=std::sin(view.pitch*radians);
    float forward=(x*cy+y*sy)*cp+z*sp;
    // Native projection divides by W without rejecting labels behind the FPS eye.
    if (forward<8) return false;
    float right=-x*sy+y*cy, up=-(x*cy+y*sy)*sp+z*cp;
    // Retain glyphs whose anchor is just outside the visible viewport.
    float height=forward*std::tan(view.verticalFov*radians*0.5f)*1.1f;
    return std::abs(up)<=height && std::abs(right)<=height*view.aspect;
}
}
