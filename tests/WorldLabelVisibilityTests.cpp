#include "../src/presentation/WorldLabelVisibility.hpp"
#include <cassert>
#include <limits>
#include <cstdio>

int main() {
    WorldLabelView view;view.active=true;view.aspect=16.f/9;view.eye[2]=100;
    float ahead[]={400,0,130},behind[]={-400,0,130},far[]={2000,0,130},side[]={400,900,130};
    assert(WorldLabelVisibility::Visible(view,ahead));
    assert(!WorldLabelVisibility::Visible(view,behind));
    assert(!WorldLabelVisibility::Visible(view,far));
    assert(!WorldLabelVisibility::Visible(view,side));
    // Rotation, height and scoping change which world labels are in view.
    view.yaw=180;assert(WorldLabelVisibility::Visible(view,behind));
    view.yaw=0;view.verticalFov=10;
    float scopeOutside[]={400,100,130};assert(!WorldLabelVisibility::Visible(view,scopeOutside));
    view.verticalFov=85;view.pitch=45;
    float above[]={300,0,400};assert(WorldLabelVisibility::Visible(view,above));
    float below[]={300,0,-500};assert(!WorldLabelVisibility::Visible(view,below));
    float invalid[]={std::numeric_limits<float>::quiet_NaN(),0,0};
    assert(!WorldLabelVisibility::Visible(view,invalid));
    view.active=false;assert(WorldLabelVisibility::Visible(view,far));
    assert(WorldLabelVisibility::Visible(view,behind));
    std::puts("World label visibility tests passed.");
}
