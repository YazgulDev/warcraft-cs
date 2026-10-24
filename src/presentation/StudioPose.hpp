#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <vector>

// WCG3 stores local rigid bones. Interpolate rotations before composing parents: matrix lerp bends limbs.
namespace StudioPose {
using Matrix=std::array<float,12>;
using Quaternion=std::array<float,4>;
inline Quaternion Rotation(const float* m) {
    Quaternion q{};float trace=m[0]+m[5]+m[10];
    if(trace>0){float s=std::sqrt(trace+1)*2;q={(m[9]-m[6])/s,(m[2]-m[8])/s,(m[4]-m[1])/s,s*.25f};}
    else {
        int i=m[5]>m[0] ? 1 : 0;if(m[10]>m[i*4+i])i=2;int j=(i+1)%3,k=(j+1)%3;
        float s=std::sqrt(std::max(0.f,1+m[i*4+i]-m[j*4+j]-m[k*4+k]))*2;
        q[i]=s*.25f;q[j]=(m[j*4+i]+m[i*4+j])/s;q[k]=(m[k*4+i]+m[i*4+k])/s;q[3]=(m[k*4+j]-m[j*4+k])/s;
    }
    return q;
}
inline Matrix Blend(const float* a,const float* b,float weight) {
    auto qa=Rotation(a),qb=Rotation(b);float dot=0;for(int i=0;i<4;++i)dot+=qa[i]*qb[i];
    if(dot<0){dot=-dot;for(auto& value:qb)value=-value;}
    float left=1-weight,right=weight;
    if(dot<.9995f){float angle=std::acos(std::clamp(dot,-1.f,1.f)),s=std::sin(angle);left=std::sin((1-weight)*angle)/s;right=std::sin(weight*angle)/s;}
    Quaternion q{};float length=0;for(int i=0;i<4;++i){q[i]=qa[i]*left+qb[i]*right;length+=q[i]*q[i];}
    length=std::sqrt(length);for(auto& v:q)v/=length;float x=q[0],y=q[1],z=q[2],w=q[3];
    Matrix m={1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),0,2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),0,
        2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y),0};
    for(int i:{3,7,11})m[i]=a[i]+(b[i]-a[i])*weight;return m;
}
inline Matrix Compose(const float* parent,const Matrix& local) {
    Matrix m{};for(int row=0;row<3;++row)for(int col=0;col<4;++col) {
        for(int k=0;k<3;++k)m[row*4+col]+=parent[row*4+k]*local[k*4+col];
        if(col==3)m[row*4+col]+=parent[row*4+3];
    }return m;
}
inline std::vector<float> Sample(const float* a,const float* b,float weight,const std::vector<int>& parents) {
    std::vector<float> result(parents.size()*12);
    for(size_t i=0;i<parents.size();++i){auto matrix=Blend(a+i*12,b+i*12,weight);
        if(parents[i]>=0)matrix=Compose(result.data()+parents[i]*12,matrix);
        std::copy(matrix.begin(),matrix.end(),result.begin()+i*12);
    }return result;
}
}
