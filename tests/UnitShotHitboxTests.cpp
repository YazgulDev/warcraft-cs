#include "MdxHitFixture.hpp"
#include "../src/combat/UnitHitboxes.hpp"
#include "../src/geometry/ModelBounds.hpp"
#include <cassert>
#include <cstdio>

namespace {
alignas(4) unsigned char object[0x100]={},sprite[0x100]={},model[0x100]={},animation[0x100]={};
uintptr_t table[32]={};
float basis[]={0,1,0,-1,0,0,0,0,1},position[]={1000,2000,200},visualScale=2;
float* __fastcall MatrixGetter(uintptr_t,uintptr_t,float* out){std::memcpy(out,basis,sizeof(basis));return out;}
float* __fastcall PositionGetter(uintptr_t,uintptr_t,float* out){std::memcpy(out,position,sizeof(position));return out;}
float __fastcall ScaleGetter(uintptr_t,uintptr_t){return visualScale;}
unsigned clocks[4]={500};bool building=false;
int candidateLogs=0;
wc3::Bits RealBits(float f){wc3::Bits bits;std::memcpy(&bits,&f,4);return bits;}
}
namespace wc3 {
void Log(const char* format,...){if(std::strstr(format,"shot candidate"))++candidateLogs;}
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
    *reinterpret_cast<uintptr_t*>(sprite)=reinterpret_cast<uintptr_t>(table);
    table[0x38/4]=reinterpret_cast<uintptr_t>(MatrixGetter);table[0x3C/4]=reinterpret_cast<uintptr_t>(PositionGetter);
    table[0x44/4]=reinterpret_cast<uintptr_t>(ScaleGetter);
    *reinterpret_cast<float*>(sprite+0xE8)=2;*reinterpret_cast<float*>(sprite+0xC8)=200;
    *reinterpret_cast<uintptr_t*>(model+0x98)=reinterpret_cast<uintptr_t>(animation);
    *reinterpret_cast<uintptr_t*>(animation+8)=reinterpret_cast<uintptr_t>(clocks);*reinterpret_cast<unsigned*>(animation+0x0C)=1;
    UnitHitboxes boxes;boxes.Configure(reinterpret_cast<uintptr_t>(image));float origin[]={1000,1800,420},ray[]={0,1,0},entry=0;bool head=false;
    assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && head && std::abs(entry-220)<.001f);
    // Only an actual-shot opt-in emits the bounded candidate record; reticle queries stay quiet.
    assert(candidateLogs==0);
    assert(boxes.IntersectShot(1,origin,ray,500,entry,head,.35f,true) && candidateLogs==1);
    clocks[0]=0;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && head && std::abs(entry-200)<.001f);
    clocks[0]=1000;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && head && std::abs(entry-240)<.001f);
    // Native render position and the complete independently scaled/tilted basis win over JASS facing/XY.
    position[0]=1013;position[1]=2041;position[2]=230;
    const float tilted[]={0,1,0,-1,0,0,.5f,0,1};std::memcpy(basis,tilted,sizeof(basis));
    float renderedOrigin[]={1123,1841,450};clocks[0]=500;
    assert(boxes.IntersectShot(1,renderedOrigin,ray,500,entry,head) && head && std::abs(entry-220)<.001f);
    // A ray above/between the real surfaces must stay a miss instead of selecting a broad-box neighbor.
    renderedOrigin[2]=600;assert(!boxes.IntersectShot(1,renderedOrigin,ray,500,entry,head));
    position[0]=1000;position[1]=2000;position[2]=200;const float flat[]={0,1,0,-1,0,0,0,0,1};std::memcpy(basis,flat,sizeof(basis));
    building=true;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && !head);building=false;
    clocks[0]=999999;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && !head);
    clocks[0]=500;image[0x512500]=0;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && !head);
    image[0x512500]=0x8B;visualScale=0;assert(boxes.IntersectShot(1,origin,ray,500,entry,head) && !head);
    boxes.Reset();VirtualFree(image,0,MEM_RELEASE);
    std::puts("Production shot hitboxes: native pose, full render basis/position/scale, empty space, structure exclusion and safe unverified fallback passed.");
}
