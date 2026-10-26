#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// Classic MDX tracks and pivot transforms are portable data rules, independent of native pointers.
namespace MdxPose {
using Value=std::array<float,4>;
using Matrix=std::array<float,16>;
inline Value Normalize(Value q) {
    float length=0;for(float v:q)length+=v*v;
    if(length<.000001f)return {0,0,0,1};
    for(float& v:q)v/=std::sqrt(length);return q;
}
inline Value Slerp(Value a,Value b,float t) {
    a=Normalize(a);b=Normalize(b);float dot=0;for(int i=0;i<4;++i)dot+=a[i]*b[i];
    if(dot<0){for(float& v:b)v=-v;dot=-dot;}
    float x=1-t,y=t;
    if(dot<.9995f){float angle=std::acos(std::clamp(dot,-1.f,1.f));x=std::sin((1-t)*angle)/std::sin(angle);y=std::sin(t*angle)/std::sin(angle);}
    Value result;for(int i=0;i<4;++i)result[i]=a[i]*x+b[i]*y;return Normalize(result);
}
struct Key {uint32_t time=0;Value value{},in{},out{};};
struct Track {
    unsigned interpolation=0,global=0xffffffff;
    std::vector<Key> keys;
    Value Sample(float time,unsigned start,unsigned end,Value fallback,bool rotation=false) const {
        auto first=std::lower_bound(keys.begin(),keys.end(),start,[](const Key& k,unsigned t){return k.time<t;});
        auto finish=std::upper_bound(first,keys.end(),end,[](unsigned t,const Key& k){return t<k.time;});
        if(first==finish)return fallback;
        auto right=std::upper_bound(first,finish,time,[](float t,const Key& k){return t<k.time;});
        if(right==first)return first->value;
        auto left=right-1;if(right==finish || interpolation==0)return left->value;
        float t=std::clamp((time-left->time)/float(right->time-left->time),0.f,1.f);
        if(rotation) {
            auto result=Slerp(left->value,right->value,t);
            return interpolation<2 ? result : Slerp(result,Slerp(left->out,right->in,t),2*t*(1-t));
        }
        Value result{};
        for(int i=0;i<3;++i) {
            if(interpolation==1)result[i]=left->value[i]*(1-t)+right->value[i]*t;
            else if(interpolation==2)result[i]=(2*t*t*t-3*t*t+1)*left->value[i]+(t*t*t-2*t*t+t)*left->out[i]+
                (-2*t*t*t+3*t*t)*right->value[i]+(t*t*t-t*t)*right->in[i];
            else result[i]=(1-t)*(1-t)*(1-t)*left->value[i]+3*t*(1-t)*(1-t)*left->out[i]+
                3*t*t*(1-t)*right->in[i]+t*t*t*right->value[i];
        }
        return result;
    }
};
inline Matrix Identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
inline Matrix Multiply(const Matrix& a,const Matrix& b) {
    Matrix result{};for(int r=0;r<4;++r)for(int c=0;c<4;++c)for(int k=0;k<4;++k)result[r*4+c]+=a[r*4+k]*b[k*4+c];return result;
}
inline Matrix Transform(const float* pivot,Value translation,Value rotation,Value scale) {
    auto q=Normalize(rotation);float x=q[0],y=q[1],z=q[2],w=q[3];
    Matrix result={1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w,0,
        2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w,0,
        2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y,0,0,0,0,1};
    for(int r=0;r<3;++r){for(int c=0;c<3;++c)result[r*4+c]*=scale[c];
        result[r*4+3]=pivot[r]+translation[r];for(int c=0;c<3;++c)result[r*4+3]-=result[r*4+c]*pivot[c];}
    return result;
}
inline std::array<float,3> Point(const Matrix& matrix,const float* point) {
    std::array<float,3> result{};for(int r=0;r<3;++r){result[r]=matrix[r*4+3];for(int c=0;c<3;++c)result[r]+=matrix[r*4+c]*point[c];}return result;
}
inline bool Triangle(const float* a,const float* b,const float* c,const float* origin,const float* direction,float limit,float& entry) {
    float e[3],f[3],p[3],t[3],q[3];for(int i=0;i<3;++i){e[i]=b[i]-a[i];f[i]=c[i]-a[i];t[i]=origin[i]-a[i];}
    p[0]=direction[1]*f[2]-direction[2]*f[1];p[1]=direction[2]*f[0]-direction[0]*f[2];p[2]=direction[0]*f[1]-direction[1]*f[0];
    float determinant=e[0]*p[0]+e[1]*p[1]+e[2]*p[2];if(std::abs(determinant)<.000001f)return false;
    float u=(t[0]*p[0]+t[1]*p[1]+t[2]*p[2])/determinant;if(u<0 || u>1)return false;
    q[0]=t[1]*e[2]-t[2]*e[1];q[1]=t[2]*e[0]-t[0]*e[2];q[2]=t[0]*e[1]-t[1]*e[0];
    float v=(direction[0]*q[0]+direction[1]*q[1]+direction[2]*q[2])/determinant;if(v<0 || u+v>1)return false;
    float distance=(f[0]*q[0]+f[1]*q[1]+f[2]*q[2])/determinant;
    if(distance<0 || distance>=limit)return false;entry=distance;return true;
}
}
