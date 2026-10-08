#include "../src/FpsProjection.hpp"
#include <cassert>
#include <cstdio>

int main() {
    // Live native trace: the world camera starts at 100/5000, the portrait at 8/1000.
    assert(FpsProjection::NearPlane(true,100,FpsProjection::WorldFarClip)==8);
    assert(FpsProjection::NearPlane(true,8,1000)==8);
    // Other cameras and every RTS/menu/cinematic view retain the engine's clipping plane.
    assert(FpsProjection::NearPlane(true,100,1000)==100);
    assert(FpsProjection::NearPlane(false,100,FpsProjection::WorldFarClip)==100);
    assert(FpsProjection::NearPlane(true,2,FpsProjection::WorldFarClip)==2);
    std::puts("FPS ground clipping, native portrait and RTS projection checks passed.");
}
