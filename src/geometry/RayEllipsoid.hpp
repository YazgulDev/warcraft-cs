#pragma once
#include "RayBounds.hpp"

// Unknown creatures retain a bounded body fallback; empty corners of a broad box are not physical contacts.
inline bool IntersectEllipsoid(const Bounds3& bounds,const float* origin,const float* direction,float limit,float& entry) {
    float a=0,b=0,c=-1;
    for(int i=0;i<3;++i) {
        float radius=(bounds.maximum[i]-bounds.minimum[i])*.5f;if(radius<=.0001f)return false;
        float o=(origin[i]-(bounds.minimum[i]+bounds.maximum[i])*.5f)/radius,d=direction[i]/radius;
        a+=d*d;b+=o*d;c+=o*o;
    }
    if(a<.00000001f)return false;float discriminant=b*b-a*c;if(discriminant<0)return false;
    float nearDistance=(-b-std::sqrt(discriminant))/a,farDistance=(-b+std::sqrt(discriminant))/a;
    if(farDistance<0)return false;entry=std::max(0.f,nearDistance);return entry<limit;
}
