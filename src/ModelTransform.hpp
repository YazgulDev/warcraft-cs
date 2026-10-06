#pragma once
#include <cmath>

struct ModelTransform {
    float matrix[9] = {1,0,0,0,1,0,0,0,1};
    float position[3] = {};
    // Warcraft stores basis rows for row-vector multiplication; ray math uses column vectors.
    void RendererMatrix(const float* values) {
        for (unsigned row=0;row<3;++row)
            for (unsigned col=0;col<3;++col) matrix[row*3+col]=values[col*3+row];
    }
    // Invert the full render transform, retaining independent scale, tilt and native visual height.
    bool LocalRay(const float* origin, const float* direction, float* localOrigin, float* localDirection) const {
        for (float value : matrix) if (!std::isfinite(value)) return false;
        for (float value : position) if (!std::isfinite(value)) return false;
        const float* m=matrix;
        float inverse[]={m[4]*m[8]-m[5]*m[7],m[2]*m[7]-m[1]*m[8],m[1]*m[5]-m[2]*m[4],
            m[5]*m[6]-m[3]*m[8],m[0]*m[8]-m[2]*m[6],m[2]*m[3]-m[0]*m[5],
            m[3]*m[7]-m[4]*m[6],m[1]*m[6]-m[0]*m[7],m[0]*m[4]-m[1]*m[3]};
        float determinant=m[0]*inverse[0]+m[1]*inverse[3]+m[2]*inverse[6];
        if (!std::isfinite(determinant)||std::abs(determinant)<0.000001f) return false;
        for (unsigned row=0;row<3;++row) {
            localOrigin[row]=localDirection[row]=0;
            for (unsigned col=0;col<3;++col) {
                float value=inverse[row*3+col]/determinant;
                localOrigin[row]+=value*(origin[col]-position[col]);
                localDirection[row]+=value*direction[col];
            }
        }
        // Do not normalize the transformed direction: ray entry must remain in world-distance units.
        return true;
    }
};
