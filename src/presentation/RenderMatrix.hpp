#pragma once
#include <array>
#include <cmath>

// Column-major matrices keep existing view/model math identical on both graphics APIs.
namespace RenderMatrix {
using Matrix=std::array<float,16>;
inline Matrix Identity() {return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
inline Matrix Multiply(const Matrix& a,const Matrix& b) {
    Matrix result={};
    for(int column=0;column<4;++column)for(int row=0;row<4;++row)
        for(int k=0;k<4;++k)result[column*4+row]+=a[k*4+row]*b[column*4+k];
    return result;
}
inline Matrix DepthForD3D(Matrix matrix) {
    // GL clip depth is -w..w; D3D clip depth is 0..w. X/Y and perspective W are unchanged.
    for(int row=0;row<4;++row)matrix[row*4+2]=(matrix[row*4+2]+matrix[row*4+3])*.5f;
    return matrix;
}
}
