#include "../src/presentation/ViewmodelMotion.hpp"
#include "../src/presentation/MuzzleFlashPulse.hpp"
#include <cassert>

static float Magnitude(const ViewmodelPose& p){return std::abs(p.x)+std::abs(p.y)+std::abs(p.z)+std::abs(p.roll);}
int main() {
    ViewmodelSettings settings;ViewmodelMotion still,run,walk,duck,slowFrames,fastFrames;
    for(int i=0;i<144;++i) {
        still.Step(1.f/144,0,true,0,settings);run.Step(1.f/144,250,true,0,settings);
        walk.Step(1.f/144,125,true,0,settings);duck.Step(1.f/144,250,true,1,settings);
    }
    assert(Magnitude(still.Pose())==0 && Magnitude(run.Pose())>.1f);
    assert(Magnitude(duck.Pose())<Magnitude(run.Pose()));
    // Integrating the same elapsed time must not change motion with the render FPS.
    for(int i=0;i<30;++i)slowFrames.Step(1.f/30,250,true,0,settings);
    for(int i=0;i<144;++i)fastFrames.Step(1.f/144,250,true,0,settings);
    assert(std::abs(slowFrames.Pose().x-fastFrames.Pose().x)<.0001f);
    assert(std::abs(slowFrames.Pose().roll-fastFrames.Pose().roll)<.0001f);
    for(int i=0;i<144;++i)run.Step(1.f/144,0,true,0,settings);
    assert(Magnitude(run.Pose())<.001f);
    for(int i=0;i<144;++i)walk.Step(1.f/144,250,false,0,settings);
    assert(Magnitude(walk.Pose())<.001f);
    settings.bob=false;duck.Step(.01f,250,true,0,settings);assert(Magnitude(duck.Pose())==0);
    // Short flashes begin on presentation, expire once and never restart on redraw or a settings toggle.
    MuzzleFlashPulse flash;assert(flash.Sample(0,false,100,65)==0);
    assert(flash.Sample(1,true,1000,65)==1 && flash.Sample(1,true,1030,65)>0);
    assert(flash.Sample(1,true,1065,65)==0 && flash.Sample(1,true,2000,65)==0);
    assert(flash.Sample(2,true,2000,65)==1);
    assert(flash.Sample(3,false,2010,65)==0 && flash.Sample(3,true,2020,65)==0);
    assert(flash.Sample(4,true,0xfffffff0u,65)==1 && flash.Sample(4,true,20,65)>0);
    assert(flash.Sample(4,true,60,65)==0);
}
