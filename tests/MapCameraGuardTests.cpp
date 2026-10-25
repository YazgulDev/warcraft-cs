#include "../src/platform/MapCameraGuard.hpp"
#include "../src/platform/WarcraftApi.hpp"
#include "../src/presentation/FirstPersonCamera.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
using ClampHook=float* (__fastcall*)(uintptr_t,uintptr_t,float*,const float*);
ClampHook renderClamp=nullptr;
alignas(4) unsigned char ui[0x300]={},camera[0x600]={},otherCamera[0x600]={};
uintptr_t image=0;
bool ownsCamera=true;
float fields[7]={100},target[3]={};
const float authoredLimits[4]={-512,-768,512,768};
wc3::Bits Bits(float f){wc3::Bits bits;std::memcpy(&bits,&f,4);return bits;}
float* __fastcall NativeClamp(uintptr_t object,uintptr_t,float* out,const float* in) {
    auto limits=reinterpret_cast<float*>(object+0x4CC);
    // Model the native transformed clamp frame, including its inverse. A world-space
    // rectangle substituted for these values would fail the rotated-edge checks.
    constexpr float c=.8f,s=.6f;
    float u=std::clamp(in[0]*c-in[1]*s,limits[1],limits[3]);
    float v=std::clamp(in[0]*s+in[1]*c,limits[0],limits[2]);
    out[0]=u*c+v*s;out[1]=-u*s+v*c;out[2]=0;return out;
}
bool Hook(void* address,void* detour,void** original) {
    if(reinterpret_cast<uintptr_t>(address)==image+0x303070){
        renderClamp=reinterpret_cast<ClampHook>(detour);*original=reinterpret_cast<void*>(NativeClamp);
    }
    return true;
}
void __cdecl Position(float* x,float* y) {
    float in[3]={*x,*y,0};NativeClamp(reinterpret_cast<uintptr_t>(camera),0,target,in);
}
void __cdecl Field(int field,float* value,float*){
    fields[field]=*value;
    // Native field/orientation changes can revalidate an earlier position write.
    float result[3];NativeClamp(reinterpret_cast<uintptr_t>(camera),0,result,target);
    std::memcpy(target,result,sizeof(target));
}
wc3::Bits __cdecl GetField(int field){return Bits(fields[field]);}
wc3::Bits __cdecl TargetZ(){return Bits(fields[6]);}
void Retick() {
    float result[3];renderClamp(reinterpret_cast<uintptr_t>(camera),0,result,target);
    std::memcpy(target,result,sizeof(target));
    assert(!std::memcmp(camera+0x4CC,authoredLimits,sizeof(authoredLimits)));
}
void Near(float a,float b){assert(std::abs(a-b)<.02f);}
}
namespace wc3 {
void Log(const char*,...){}
float Real(Bits bits){float f;std::memcpy(&f,&bits,4);return f;}
void (__cdecl* SetCameraPosition)(float*,float*)=Position;
void (__cdecl* SetCameraField)(int,float*,float*)=Field;
Bits (__cdecl* GetCameraField)(int)=GetField;
Bits (__cdecl* GetCameraTargetPositionZ)()=TargetZ;
}
int main() {
    // A private executable image substitutes only the owned native entry signatures/UI
    // accessor. Tests never load, inject into or modify an installed Warcraft process.
    image=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x400000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(image);
    const unsigned char getter[]={0x8B,0x81,0x54,0x02,0x00,0x00,0xC3};
    const unsigned char minX[]={0xD9,0x80,0xD0,0x04,0x00,0x00};
    const unsigned char entry[]={0x83,0xEC,0x14,0xD9,0xEE,0x8B,0x44,0x24,0x1C};
    const unsigned char clampX[]={0xD9,0x81,0xD0,0x04,0x00,0x00};
    std::memcpy(reinterpret_cast<void*>(image+0x2F5ED0),getter,sizeof(getter));
    std::memcpy(reinterpret_cast<void*>(image+0x3B4C62),minX,sizeof(minX));
    std::memcpy(reinterpret_cast<void*>(image+0x303070),entry,sizeof(entry));
    std::memcpy(reinterpret_cast<void*>(image+0x303107),clampX,sizeof(clampX));
    auto thunk=reinterpret_cast<unsigned char*>(image+0x300710);thunk[0]=0xB8;
    uintptr_t uiAddress=reinterpret_cast<uintptr_t>(ui);std::memcpy(thunk+1,&uiAddress,4);thunk[5]=0xC3;
    *reinterpret_cast<uintptr_t*>(ui+0x254)=reinterpret_cast<uintptr_t>(camera);
    std::memcpy(camera+0x4CC,authoredLimits,sizeof(authoredLimits));
    std::memcpy(otherCamera+0x4CC,authoredLimits,sizeof(authoredLimits));
    MapCameraGuard::Install(image,[](){return ownsCamera;},Hook);assert(renderClamp);
    FirstPersonCamera view;
    for(float x:{-4000.f,0.f,4000.f})for(float y:{-4000.f,0.f,4000.f})
        for(float yaw:{0.f,60.f,180.f,270.f,720.f})for(float pitch:{-65.f,0.f,65.f}) {
            ownsCamera=false;view.Update(x,y,200,yaw,pitch,90,15000);ownsCamera=true;
            // Repeated engine updates used to clamp this target after the synchronous write.
            for(int frame=0;frame<3;++frame)Retick();
            constexpr float r=.01745329252f;
            Near(target[0]-fields[0]*std::cos(pitch*r)*std::cos(yaw*r),x);
            Near(target[1]-fields[0]*std::cos(pitch*r)*std::sin(yaw*r),y);
        }
    ownsCamera=false;Retick();assert(std::hypot(target[0],target[1])<1000);
    ownsCamera=true;float in[3]={4000,4000,0},out[3];
    renderClamp(reinterpret_cast<uintptr_t>(otherCamera),0,out,in);
    assert(std::hypot(out[0],out[1])<1000); // Unrelated cameras retain native restrictions.
    *reinterpret_cast<unsigned char*>(image+0x303107)=0;renderClamp=nullptr;
    MapCameraGuard::Install(image,[](){return ownsCamera;},Hook);assert(!renderClamp);
    VirtualFree(reinterpret_cast<void*>(image),0,MEM_RELEASE);
    std::puts("FPS edge tracking, repeated render clamps, rotated bounds, RTS/unrelated camera isolation and signature refusal passed.");
}
