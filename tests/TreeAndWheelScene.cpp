#include "TreeAndWheelScene.hpp"
#include "../src/geometry/ModelBounds.hpp"
#include "../src/platform/SpriteTransform.hpp"
#include <fstream>

namespace {
struct Fixture { wc3::Handle handle; int type; };
std::vector<Fixture> fixtures;
DWORD verifyAt=0;
bool Transform(wc3::Handle handle,uintptr_t base,ModelTransform& transform) {
    using Resolve=uintptr_t (__fastcall*)(wc3::Handle,uintptr_t);
    uintptr_t object=reinterpret_cast<Resolve>(base+0x3BE010)(handle,0),sprite=0;SIZE_T got=0;
    return object && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(object+0x28),&sprite,sizeof(sprite),&got) &&
        got==sizeof(sprite) && SpriteTransform::Read(sprite,transform);
}
void Ray(const ModelTransform& transform,float side,float height,float* origin,float* direction) {
    float local[]={-250,side,height};
    float magnitude=std::sqrt(transform.matrix[0]*transform.matrix[0]+transform.matrix[3]*transform.matrix[3]+transform.matrix[6]*transform.matrix[6]);
    for (unsigned r=0;r<3;++r) {
        origin[r]=transform.position[r];for (unsigned c=0;c<3;++c) origin[r]+=transform.matrix[r*3+c]*local[c];
        direction[r]=transform.matrix[r*3]/magnitude;
    }
}
void Cleanup() {
    for (const auto& fixture:fixtures) if (wc3::GetDestructableTypeId(fixture.handle)) wc3::RemoveDestructable(fixture.handle);
    fixtures.clear();verifyAt=0;
}
}
void TreeAndWheelScene::Tick(ShooterController& controller,uintptr_t base,const char* root) {
    if (!controller.Visible()) return;
    static DWORD check=0;DWORD now=GetTickCount();if (now-check<100) return;check=now;
    auto unit=controller.TestUnit();std::string path=std::string(root)+"\\test-tree-wheel.request";
    if (GetFileAttributesA(path.c_str())!=INVALID_FILE_ATTRIBUTES) {
        std::ifstream file(path);std::string command;std::getline(file,command);file.close();DeleteFileA(path.c_str());
        if (command=="geometry") {
            // Only a disposable custom map receives these explicit tree/gate fixtures; no campaign is loaded.
            Cleanup();float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit));
            const int types[]={0x4C546C74,0x59546374,0x4C546531};
            for (int type:types) for (float scale:{1.f,2.f}) {
                float dx=x+400+float(fixtures.size())*100,dy=y-350,facing=45;
                auto handle=wc3::CreateDestructable(type,&dx,&dy,&facing,&scale,0);fixtures.push_back({handle,type});
            }
            // Let Warcraft initialize the rendered sprite transform before testing its world-space geometry.
            verifyAt=now+400;
        } else if (command=="state") {
            wc3::Log("tree-wheel fixture state weapon=%s slot=%d scope=%d ammo=%d reserve=%d",
                ShooterController::weapons[controller.WeaponIndex()].name,controller.WeaponIndex()+1,controller.Scoped(),controller.Ammo(),controller.Reserve());
        } else if (command=="cleanup") Cleanup();
    }
    if (verifyAt && now>=verifyAt) {
        verifyAt=0;DestructableHitboxes hitboxes;hitboxes.Configure(base);
        for (const auto& fixture:fixtures) {
            Bounds3 bounds;ModelTransform transform;
            if (!wc3::GetDestructableTypeId(fixture.handle) || !ModelBounds::Load(wc3::DestructableModelPath(fixture.type),bounds) || !Transform(fixture.handle,base,transform)) {
                wc3::Log("tree-wheel fixture FAIL model/transform type=%08X",fixture.type);continue;
            }
            float origin[3],direction[3],entry=0;
            float centerY=(bounds.minimum[1]+bounds.maximum[1])*.5f;
            float height=fixture.type==0x4C546531 ? (bounds.minimum[2]+bounds.maximum[2])*.5f : 96.f;
            Ray(transform,centerY,height,origin,direction);
            bool direct=hitboxes.Intersect(fixture.handle,origin,direction,2000,entry);
            // A ray below the canopy and well beside the narrow trunk lies inside the old full-model box.
            bool side=false;
            if (fixture.type!=0x4C546531) { Ray(transform,bounds.maximum[1]*.8f,height,origin,direction);side=hitboxes.Intersect(fixture.handle,origin,direction,2000,entry); }
            wc3::Log("tree-wheel fixture %s type=%08X direct=%d besideTrunk=%d worldEntry=%.1f",
                direct && !side ? "PASS" : "FAIL",fixture.type,direct,side,entry);
        }
    }
}
