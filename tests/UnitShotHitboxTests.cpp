#include "MdxHitFixture.hpp"
#include "../src/combat/UnitHitboxes.hpp"
#include "../src/geometry/ModelBounds.hpp"
#include <cassert>
#include <cstdio>

namespace {
alignas(4) unsigned char object[0x100]={},sprite[0x100]={},model[0x100]={},animation[0x100]={};
unsigned clocks[4]={500};bool building=false;
wc3::Bits RealBits(float f){wc3::Bits bits;std::memcpy(&bits,&f,4);return bits;}
}
namespace wc3 {
void Log(const char*,...){}
float Real(Bits bits){float f;std::memcpy(&f,&bits,4);return f;}
std::string UnitModelPath(int){return "synthetic.mdx";}
float Ground(float,float){return 10;}
int (__cdecl* GetUnitTypeId)(Handle)=[](Handle)->int{return 1;};
BOOL (__cdecl* IsUnitType)(Handle,int)=[](Handle,int type)->BOOL{return type==2 && building;};
Bits (__cdecl* GetUnitX)(Handle)=[](Handle)->Bits{return RealBits(1000);};
Bits (__cdecl* GetUnitY)(Handle)=[](Handle)->Bits{return RealBits(2000);};
Bits (__cdecl* GetUnitFacing)(Handle)=[](Handle)->Bits{return RealBits(90);};
Bits (__cdecl* GetUnitFlyHeight)(Handle)=[](Handle)->Bits{return RealBits(50);};
}
namespace ModelBounds {
bool Load(std::string,Bounds3& b,TreeTrunkMesh*){b={{-100,-100,-10},{100,100,200}};return true;}
bool ReadModel(std::string,std::vector<unsigned char>& b){b=MdxHitFixture::Model();return true;}
}
int main() {
    _set_error_mode(_OUT_TO_STDERR);
    // Fake only verified native identity/clock fields; production unit transforms and MDX tracing run unchanged.
    auto image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x520000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(image);
    image[0x3BDCB0]=0xB8;auto obj=reinterpret_cast<uintptr_t>(object);std::memcpy(image+0x3BDCB1,&obj,4);image[0x3BDCB5]=0xC3;
    const unsigned char signature[]={0x8B,0x41,0x44,0x83,0x49,0x54,0x20};std::memcpy(image+0x512500,signature,sizeof(signature));
    *reinterpret_cast<uintptr_t*>(object+0x28)=reinterpret_cast<uintptr_t>(sprite);
    *reinterpret_cast<uintptr_t*>(sprite+0x20)=reinterpret_cast<uintptr_t>(model);
    *reinterpret_cast<float*>(sprite+0xE8)=2;*reinterpret_cast<float*>(sprite+0xC8)=200;
    *reinterpret_cast<uintptr_t*>(model+0x98)=reinterpret_cast<uintptr_t>(animation);
    *reinterpret_cast<uintptr_t*>(animation+8)=reinterpret_cast<uintptr_t>(clocks);*reinterpret_cast<unsigned*>(animation+0x0C)=1;
    UnitHitboxes boxes;boxes.Configure(reinterpret_cast<uintptr_t>(image));float origin[]={1000,1800,420},ray[]={0,1,0},entry=0;bool head=false;
    assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && head && std::abs(entry-220)<.001f);
    clocks[0]=0;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && head && std::abs(entry-200)<.001f);
    clocks[0]=1000;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && head && std::abs(entry-240)<.001f);
    building=true;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && !head);building=false;
    clocks[0]=999999;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && !head);
    clocks[0]=500;image[0x512500]=0;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && !head);
    boxes.Reset();VirtualFree(image,0,MEM_RELEASE);
    std::puts("Production shot hitboxes: native pose time, facing, scale, rendered height, structure exclusion and safe unverified fallback passed.");
}
