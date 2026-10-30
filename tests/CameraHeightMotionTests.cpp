#include "../src/presentation/CameraHeightMotion.hpp"
#include <cassert>
#include <cstdio>

int main() {
    CameraHeightSettings settings;CameraHeightMotion motion;
    motion.Step(0,0,0,true,1.f/60,27,settings);
    motion.Step(4,0,20,true,1.f/60,27,settings);
    // A real ground step no longer moves the displayed eye by the entire step in one render.
    assert(20+motion.Offset()>0 && 20+motion.Offset()<3);
    for(int i=0;i<120;++i)motion.Step(4,0,20,true,1.f/60,27,settings);
    assert(std::abs(motion.Offset())<.001f);
    motion.Step(8,0,0,true,1.f/60,27,settings);
    assert(motion.Offset()>12 && motion.EyeOffset(96,108)==12 && motion.EyeOffset(54,66)==12);
    // Linear slope response is independent of render rate; reversals stay within the existing step bound.
    float result[3]={};int j=0;
    for(int fps:{30,60,144}) {
        motion.Reset();motion.Step(0,0,0,true,0,27,settings);
        for(int i=1;i<=fps;++i)motion.Step(i*200.f/fps,0,i*40.f/fps,true,1.f/fps,27,settings);
        result[j++]=40+motion.Offset();
    }
    assert(std::abs(result[0]-result[2])<.001f && std::abs(result[1]-result[2])<.001f);
    for(int i=0;i<100;++i) {motion.Step(200.f+i,0,i%2 ? 20.f : 0.f,true,1.f/144,27,settings);assert(std::abs(motion.Offset())<=27);}
    // Existing correction decays during a jump; the physical ascent/fall itself is never filtered.
    motion.Reset();motion.Step(0,0,0,false,0,27,settings);
    for(int i=1;i<60;++i){motion.Step(float(i),0,i<30 ? float(i*4) : float(240-i*4),false,1.f/60,27,settings);assert(motion.Offset()==0);}
    motion.Step(1000,0,0,true,1.f/60,27,settings);assert(motion.Offset()==0);
    motion.Step(1004,0,20,true,1.f/60,27,settings);assert(motion.Offset()<0);
    motion.Step(3000,0,20,true,1.f/60,27,settings);assert(motion.Offset()==0); // Teleport/rescue discards old correction.
    settings.enabled=false;motion.Step(3004,0,40,true,1.f/60,27,settings);assert(motion.Offset()==0);
    motion.Reset();assert(motion.Offset()==0);
    std::puts("Ground step smoothing, frame-rate agreement, bounds, jump isolation, teleport reset and opt-out passed.");
}
