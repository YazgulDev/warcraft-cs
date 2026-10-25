#pragma once
#include "RayBounds.hpp"

namespace CylinderHull {
// A round foot does not occupy the empty corners of an expanded rectangle.
inline bool Overlaps(const Bounds3& box,float x,float y,float radius) {
    float dx=x-std::clamp(x,box.minimum[0],box.maximum[0]);
    float dy=y-std::clamp(y,box.minimum[1],box.maximum[1]);
    return dx*dx+dy*dy < radius*radius-.0001f;
}
inline void Normal(const Bounds3& box,float x,float y,float* normal) {
    float dx=x-std::clamp(x,box.minimum[0],box.maximum[0]);
    float dy=y-std::clamp(y,box.minimum[1],box.maximum[1]);
    float length=std::hypot(dx,dy);
    if(length>.0001f) {normal[0]=dx/length;normal[1]=dy/length;return;}
    // A native spawn can start inside a solid: the nearest face is its recovery direction.
    float distances[]={x-box.minimum[0],box.maximum[0]-x,y-box.minimum[1],box.maximum[1]-y};
    int face=int(std::min_element(distances,distances+4)-distances);
    normal[0]=face<2 ? (face ? 1.f : -1.f) : 0;
    normal[1]=face>=2 ? (face==3 ? 1.f : -1.f) : 0;
}
}
