#include "../src/platform/MovementObstacles.hpp"
#include "../src/platform/SpriteTransform.hpp"
#include "../src/platform/UnitCollisionProfile.hpp"
#include "../src/geometry/ModelBounds.hpp"
#include "../src/geometry/TreeTrunkMesh.hpp"
#include "CollisionMeshFixture.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace {
bool hasUnit=false;
int unitLoads=0,treeLoads=0;
UnitCollisionProfile collision;
bool staticFixture=false;
int meshLoads=0;float spriteX=0;
std::array<unsigned char,0x30> objectMemory={};
std::array<uintptr_t,0x48/4> spriteTable={};uintptr_t spriteMemory=0;
float __fastcall Scale(uintptr_t,uintptr_t) {return 2;}
wc3::Bits Bits(float value) { wc3::Bits result;std::memcpy(&result,&value,4);return result; }
wc3::Handle __cdecl Group() {hasUnit=true;return 10;}
void __cdecl Enum(wc3::Handle,float*,float*,float*,wc3::Handle) {}
wc3::Handle __cdecl First(wc3::Handle) {return hasUnit ? 1 : 0;}
void __cdecl Remove(wc3::Handle,wc3::Handle) {hasUnit=false;}
void __cdecl Destroy(wc3::Handle) {}
BOOL __cdecl Hidden(wc3::Handle) {return FALSE;}
wc3::Bits __cdecl State(wc3::Handle,int) {return Bits(100);}
wc3::Bits __cdecl Life(wc3::Handle) {return Bits(100);}
int __cdecl Type(wc3::Handle) {return 7;}
wc3::Bits __cdecl Position(wc3::Handle) {return Bits(0);}
uintptr_t __fastcall Resolve(wc3::Handle,uintptr_t) {return staticFixture ? reinterpret_cast<uintptr_t>(objectMemory.data()) : 1;}
}
namespace wc3 {
Handle (__cdecl* CreateGroup)()=Group;
void (__cdecl* GroupEnumUnitsInRange)(Handle,float*,float*,float*,Handle)=Enum;
Handle (__cdecl* FirstOfGroup)(Handle)=First;
void (__cdecl* GroupRemoveUnit)(Handle,Handle)=Remove;
void (__cdecl* DestroyGroup)(Handle)=Destroy;
BOOL (__cdecl* IsUnitHidden)(Handle)=Hidden;
BOOL (__cdecl* IsUnitLoaded)(Handle)=Hidden;
BOOL (__cdecl* IsUnitType)(Handle,int)=[](Handle,int)->BOOL{return FALSE;};
Bits (__cdecl* GetUnitX)(Handle)=Position;
Bits (__cdecl* GetUnitY)(Handle)=Position;
Bits (__cdecl* GetUnitState)(Handle,int)=State;
Bits (__cdecl* GetDestructableLife)(Handle)=Life;
int (__cdecl* GetUnitTypeId)(Handle)=Type;
int (__cdecl* GetDestructableTypeId)(Handle)=Type;
Bits (__cdecl* GetDestructableX)(Handle)=Position;
Bits (__cdecl* GetDestructableY)(Handle)=Position;
float Real(Bits value) {float result;std::memcpy(&result,&value,4);return result;}
uintptr_t UnitObject(Handle) {return 1;}
std::string UnitModelPath(int) {return "SharedTree.mdl";}
std::string DestructableModelPath(int) {return staticFixture ? "StaticColumn.mdl" : "SharedTree.mdl";}
std::vector<Handle> NearbyDestructables(float,float,float) {return {2};}
bool WalkableSurface(float,float,float&) {return false;}
void Log(const char*,...) {}
}
// Supply the live-profile boundary without retail process memory; discovery still runs through production code.
UnitCollisionProfile UnitCollisionProfile::Read(wc3::Handle,uintptr_t) {return collision;}
bool ModelBounds::Load(std::string,Bounds3& bounds,TreeTrunkMesh* tree) {
    if(staticFixture) {bounds={{-40,-40,0},{40,40,100}};return true;}
    // Zero extents stop before sprite access: this oracle checks discovery/cache routing, not fake memory.
    bounds={{0,0,0},{0,0,0}};
    if (tree) {++treeLoads;tree->Load("SharedTree.mdl",{},bounds);} else ++unitLoads;
    return true;
}
bool ModelBounds::LoadCollision(std::string,CylinderMesh& mesh) {
    assert(staticFixture);++meshLoads;AddMeshBox(mesh,{{-40,-40,0},{40,40,20}});return true;
}
bool SpriteTransform::Read(uintptr_t sprite,ModelTransform& transform) {
    assert(staticFixture && sprite==reinterpret_cast<uintptr_t>(&spriteMemory));transform.position[0]=spriteX;return true;
}
int main() {
    MovementObstacles obstacles;obstacles.Configure(reinterpret_cast<uintptr_t>(Resolve)-0x3BE010);
    assert(obstacles.Snapshot(99,0,0,600).empty());
    assert(unitLoads==1 && treeLoads==1);
    // The same model path must remain separate for an animated unit and a tree destructable.
    obstacles.Snapshot(99,0,0,600);assert(unitLoads==1 && treeLoads==1);
    obstacles.Reset();obstacles.Snapshot(99,0,0,600);assert(unitLoads==2 && treeLoads==2);
    // Non-colliding map helpers must not load a solid model; the explicit legacy opt-out still does.
    collision={true,false,0};obstacles.Reset();obstacles.Snapshot(99,0,0,600,true);
    assert(unitLoads==2 && treeLoads==3);
    obstacles.Snapshot(99,0,0,600,false);assert(unitLoads==3 && treeLoads==3);
    // Real discovery/scale/cache routing supplies detailed movement geometry while combat keeps coarse bounds.
    staticFixture=true;spriteMemory=reinterpret_cast<uintptr_t>(spriteTable.data());
    spriteTable[0x44/4]=reinterpret_cast<uintptr_t>(Scale);
    auto sprite=reinterpret_cast<uintptr_t>(&spriteMemory);std::memcpy(objectMemory.data()+0x28,&sprite,sizeof(sprite));
    obstacles.Reset();auto bounds=obstacles.Snapshot(99,0,0,600,true,true);
    assert(bounds.size()==1 && bounds[0].maximum[2]==200 && meshLoads==1);
    auto* first=obstacles.Geometry(0);assert(first);float height=0;
    assert(first->Floor(0,0,16,50,.69f,height) && height==40);
    obstacles.Snapshot(99,0,0,600,true,true);assert(obstacles.Geometry(0)==first && meshLoads==1);
    spriteX=200;obstacles.Snapshot(99,0,0,600,true,true);
    assert(obstacles.Geometry(0)!=first && meshLoads==1);
    assert(obstacles.Geometry(0)->Floor(200,0,16,50,.69f,height) && height==40);
    assert(!obstacles.Geometry(0)->Floor(0,0,16,50,.69f,height));
    obstacles.Snapshot(99,0,0,600,true);assert(!obstacles.Geometry(0) && meshLoads==1);
    obstacles.Reset();obstacles.Snapshot(99,0,0,600,true,true);assert(meshLoads==2 && obstacles.Geometry(0));
    puts("PASS movement obstacle cache: tree-only decoding, shared unit/destructable paths and map reset.");
}
