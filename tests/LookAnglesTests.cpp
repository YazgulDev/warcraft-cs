#include "../src/input/LookAngles.hpp"
#include <cstdio>
#include <cstdlib>

static void Require(bool value,const char* message) {
    if (!value) { std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1); }
}
static bool Near(float a,float b) { return std::abs(a-b)<.02f; }
int main() {
    float yaw=10,pitch=0;
    // This fast swipe used to be dropped completely by the 160-pixel cursor threshold.
    LookAngles::Apply(yaw,pitch,2000,0,.14f);
    Require(Near(yaw,90),"fast mouse swipe must contribute its entire 280-degree turn");
    yaw=1;LookAngles::Apply(yaw,pitch,20,0,.14f);
    Require(Near(yaw,358.2f),"left turn across zero must retain its 2.8-degree displacement");
    LookAngles::Apply(yaw,pitch,-20,0,.14f);
    Require(Near(yaw,1),"reversing across 360 must return to the same heading");
    yaw=123;
    for(int turn=0;turn<100;++turn) LookAngles::Apply(yaw,pitch,3600,0,.1f);
    Require(Near(yaw,123),"one hundred complete turns must not create negative or drifting yaw");
    float batched=10,split=10,dummy=0;
    LookAngles::Apply(batched,dummy,2000,0,.14f);
    for(int i=0;i<20;++i) LookAngles::Apply(split,dummy,100,0,.14f);
    Require(Near(batched,split),"coalesced raw packets and split frames must reach the same heading");
    yaw=10;pitch=0;LookAngles::Apply(yaw,pitch,100,10000,.009f);
    Require(Near(yaw,9.1f)&&Near(pitch,-65),"AWP fine sensitivity and pitch limits must remain intact");
    Require(Near(LookAngles::Normalize(-7560),0)&&Near(LookAngles::Normalize(7561),1),"normalization must support unlimited signed rotations");
    std::puts("Mouse look invariants passed: fast swipe, 0/360 crossing, repeated turns, packet batching, AWP pitch");
}
