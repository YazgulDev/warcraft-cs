#include "../src/presentation/ThirdPersonCamera.hpp"
#include <cassert>
#include <cstring>

namespace {
float ground=0,buildingEntry=0,obstacleEntry=200;bool obstacle=false;int groups=0,intersections=0;wc3::Handle candidate=0;
wc3::Bits Bits(float f) {wc3::Bits bits;memcpy(&bits,&f,4);return bits;}
wc3::Bits __cdecl Zero(wc3::Handle) {return Bits(0);}
wc3::Bits __cdecl Life(wc3::Handle,int) {return Bits(100);}
wc3::Bits __cdecl ObjectLife(wc3::Handle) {return Bits(100);}
wc3::Handle __cdecl Group() {++groups;candidate=2;return 1;}
void __cdecl Destroy(wc3::Handle) {}
void __cdecl Enum(wc3::Handle,float*,float*,float*,wc3::Handle) {}
wc3::Handle __cdecl First(wc3::Handle) {return candidate;}
void __cdecl Remove(wc3::Handle,wc3::Handle) {candidate=0;}
BOOL __cdecl Building(wc3::Handle,int) {return TRUE;}
BOOL __cdecl Hidden(wc3::Handle) {return FALSE;}
}
namespace wc3 {
float Real(Bits bits) {float f;memcpy(&f,&bits,4);return f;}
float TerrainGround(float,float) {return ground;}
std::vector<Handle> NearbyDestructables(float,float,float) {return obstacle ? std::vector<Handle>{3} : std::vector<Handle>{};}
Bits (__cdecl* GetUnitX)(Handle)=Zero;
Bits (__cdecl* GetUnitY)(Handle)=Zero;
Bits (__cdecl* GetUnitState)(Handle,int)=Life;
Bits (__cdecl* GetDestructableLife)(Handle)=ObjectLife;
Handle (__cdecl* CreateGroup)()=Group;
void (__cdecl* DestroyGroup)(Handle)=Destroy;
void (__cdecl* GroupEnumUnitsInRange)(Handle,float*,float*,float*,Handle)=Enum;
Handle (__cdecl* FirstOfGroup)(Handle)=First;
void (__cdecl* GroupRemoveUnit)(Handle,Handle)=Remove;
BOOL (__cdecl* IsUnitType)(Handle,int)=Building;
BOOL (__cdecl* IsUnitHidden)(Handle)=Hidden;
}
bool UnitHitboxes::Intersect(wc3::Handle,const float*,const float*,float limit,float& entry,float) {
    ++intersections;entry=buildingEntry;return entry<limit;
}
bool DestructableHitboxes::Intersect(wc3::Handle,const float*,const float*,float limit,float& entry) {
    entry=obstacleEntry;return entry<limit;
}
int main() {
    ThirdPersonCamera camera;ThirdPersonSettings settings;UnitHitboxes units;DestructableHitboxes objects;
    auto distance=[&] {auto pose=camera.Position(1,96,0,0,settings,units,objects);return pose.pivot[0]-pose.eye[0];};
    // Both overlapping forward boxes and rear obstacles used to collapse zoom. Default bypasses only buildings.
    for(float entry:{0.f,24.f,250.f}) {buildingEntry=entry;assert(distance()==500);}
    assert(groups==0 && intersections==0);
    settings.buildingCollision=true;buildingEntry=24;assert(distance()==8 && intersections==1);
    settings.buildingCollision=false;assert(distance()==500);
    // Gates/tree surfaces and terrain still protect the camera after the building policy changes.
    obstacle=true;assert(distance()==184);
    // A bridge/ladder's enclosing box and an already containing proxy are not walls behind the hero.
    obstacleEntry=0;assert(distance()==500);
    // An actual separate gate/tree surface and terrain still shorten the camera boom.
    obstacleEntry=200;assert(distance()==184);
    obstacle=false;ground=125;assert(distance()==0);
    ground=0;settings.distance=700;assert(distance()==700);
}
