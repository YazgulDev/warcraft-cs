#include "../src/presentation/StudioPose.hpp"
#include "../src/presentation/StudioSkin.hpp"
#include <cassert>
#include <cstdio>

int main() {
    // A rotating parent must carry a rigid child around an arc, not shorten its attachment/arm.
    const float a[]={1,0,0,0,0,1,0,0,0,0,1,0,1,0,0,10,0,1,0,0,0,0,1,0};
    const float b[]={0,-1,0,0,1,0,0,0,0,0,1,0,1,0,0,10,0,1,0,0,0,0,1,0};
    for(float weight:{0.f,.25f,.5f,.75f,1.f}) {
        auto pose=StudioPose::Sample(a,b,weight,{-1,0});float x=pose[15],y=pose[19];
        assert(std::abs(std::hypot(x,y)-10)<.0001f);
        assert(std::abs(pose[0]*pose[0]+pose[4]*pose[4]+pose[8]*pose[8]-1)<.0001f);
        assert(std::abs(x-10*std::cos(weight*1.5707963268f))<.0001f);
    }
    auto mid=StudioPose::Sample(a,b,.5f,{-1,0});assert(mid[15]>7 && mid[19]>7);
    auto plain=StudioSkin::UV(0,{1,0,0},.2f,.7f);assert(plain[0]==.2f && plain[1]==.7f);
    auto right=StudioSkin::UV(3,{1,0,0},0,0),up=StudioSkin::UV(3,{0,1,0},0,0);
    assert(right[0]==1 && right[1]==.5f && up[0]==.5f && up[1]==0);
    auto n=StudioSkin::Normal(b,1,0,0,true);assert(n[0]==1 && n[1]==0 && n[2]==0);
    std::puts("PASS rigid local bone hierarchy, quaternion arcs, unit normals and animated chrome coordinates.");
}
