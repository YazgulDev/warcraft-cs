#include "../src/presentation/FpsProjection.hpp"
#include "../src/presentation/FirstPersonCamera.hpp"
#include "../src/platform/WarcraftApi.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

namespace {
float fields[7]{};
wc3::Bits Bits(float value) {wc3::Bits bits;std::memcpy(&bits,&value,4);return bits;}
void __cdecl Set(int field,float* value,float*) {fields[field]=*value;}
wc3::Bits __cdecl Get(int field) {return Bits(fields[field]);}
wc3::Bits __cdecl TargetZ() {return Bits(fields[6]);}
}
namespace wc3 {
void (__cdecl* SetCameraField)(int,float*,float*)=Set;
Bits (__cdecl* GetCameraField)(int)=Get;
Bits (__cdecl* GetCameraTargetPositionZ)()=TargetZ;
float Real(Bits value) {float result;std::memcpy(&result,&value,4);return result;}
}
namespace MapCameraGuard {void PositionEyeTarget(float*,float*) {} bool AbsoluteEyeHeight(){return false;}}

int main() {
    // Every supported/reloaded world distance preserves nearby ground and isolates the portrait camera.
    for (float farClip:{3000.f,5000.f,10000.f,15000.f,20000.f}) {
        FirstPersonCamera{}.Update(0,0,96,0,0,85,farClip);
        assert(fields[1]==farClip && fields[3]==85 && fields[6]==96);
        assert(FpsProjection::NearPlane(true,100,farClip,farClip)==8);
        assert(FpsProjection::NearPlane(true,8,1000,farClip)==8);
        assert(FpsProjection::NearPlane(true,100,1000,farClip)==100);
        assert(FpsProjection::NearPlane(false,100,farClip,farClip)==100);
        assert(FpsProjection::NearPlane(true,2,farClip,farClip)==2);
    }
    assert(FpsProjection::NearPlane(true,100,5000,10000)==100);
    std::puts("FPS ground clipping, native portrait and RTS projection checks passed.");
}
