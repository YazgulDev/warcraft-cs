#pragma once
#include <cmath>

struct Triangle3 { float vertices[3][3] = {}; };

// Double-sided mesh picking keeps empty space between trunk surfaces transparent to shots.
inline bool IntersectTriangle(const Triangle3& triangle, const float* origin, const float* direction,
    float limit, float& entry) {
    float edge1[3], edge2[3], offset[3];
    for (int i=0;i<3;++i) {
        edge1[i]=triangle.vertices[1][i]-triangle.vertices[0][i];
        edge2[i]=triangle.vertices[2][i]-triangle.vertices[0][i];
        offset[i]=origin[i]-triangle.vertices[0][i];
    }
    float cross[]={direction[1]*edge2[2]-direction[2]*edge2[1],
        direction[2]*edge2[0]-direction[0]*edge2[2],direction[0]*edge2[1]-direction[1]*edge2[0]};
    float determinant=0;for (int i=0;i<3;++i) determinant+=edge1[i]*cross[i];
    if (!std::isfinite(determinant) || std::abs(determinant)<.000001f) return false;
    float u=0;for (int i=0;i<3;++i) u+=offset[i]*cross[i]/determinant;
    if (u<-.00001f || u>1.00001f) return false;
    float q[]={offset[1]*edge1[2]-offset[2]*edge1[1],
        offset[2]*edge1[0]-offset[0]*edge1[2],offset[0]*edge1[1]-offset[1]*edge1[0]};
    float v=0,t=0;
    for (int i=0;i<3;++i) { v+=direction[i]*q[i]/determinant; t+=edge2[i]*q[i]/determinant; }
    if (v<-.00001f || u+v>1.00001f || !std::isfinite(t) || t<0 || t>=limit) return false;
    // The local direction is deliberately unnormalized, preserving world distance under scale/rotation.
    entry=t;return true;
}
