#include "FloatingTextScene.hpp"
#include <cmath>
#include <string>

void FloatingTextScene::Tick(ShooterController& controller, uintptr_t base, const char* root) {
    static bool pending=false;
    std::string request=std::string(root)+"\\test-floating-text.request";
    if (GetFileAttributesA(request.c_str())!=INVALID_FILE_ATTRIBUTES) {
        DeleteFileA(request.c_str());
        auto group=wc3::CreateGroup();auto owner=wc3::GetLocalPlayer();
        wc3::GroupEnumUnitsOfPlayer(group,owner,0);wc3::Handle unit=0;
        while (auto candidate=wc3::FirstOfGroup(group)) {
            wc3::GroupRemoveUnit(group,candidate);
            if (!wc3::IsUnitType(candidate,2)) { unit=candidate;break; }
        }
        wc3::DestroyGroup(group);if (!unit)return;
        // Use a disposable map's owned worker position, never alter a player's saved match.
        float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit)),facing=0;
        auto actor=wc3::CreateUnit(owner,0x68666F6F,&x,&y,&facing);
        using Clear=void (__cdecl*)();using Select=void (__cdecl*)(wc3::Handle,BOOL);
        reinterpret_cast<Clear>(base+0x3BBAA0)();reinterpret_cast<Select>(base+0x3C7910)(actor,TRUE);
        controller.RequestToggle();pending=true;return;
    }
    if (!pending || !controller.Visible())return;pending=false;
    auto actor=controller.ViewActor();
    float originX=wc3::Real(wc3::GetUnitX(actor)),originY=wc3::Real(wc3::GetUnitY(actor));
    float angle=controller.ViewYaw()*0.01745329252f;
    using Create=wc3::Handle (__cdecl*)();
    using Text=void (__cdecl*)(wc3::Handle,wc3::Handle,float*);
    using Position=void (__cdecl*)(wc3::Handle,float*,float*,float*);
    // Stack JASS string wrappers are consumed synchronously, just like the effect adapter.
    const char* names[]={"+10 NEAR","+10 BEHIND","+10 FAR"};float offsets[]={250,-250,1800};
    for (int i=0;i<3;++i) {
        auto tag=reinterpret_cast<Create>(base+0x3BC580)();
        uintptr_t node[8]={};node[7]=reinterpret_cast<uintptr_t>(names[i]);
        uintptr_t wrapper[3]={};wrapper[2]=reinterpret_cast<uintptr_t>(node);
        float size=0.025f,x=originX+offsets[i]*std::cos(angle),y=originY+offsets[i]*std::sin(angle),height=100;
        reinterpret_cast<Text>(base+0x3BC5D0)(tag,reinterpret_cast<wc3::Handle>(wrapper),&size);
        reinterpret_cast<Position>(base+0x3BC610)(tag,&x,&y,&height);
        wc3::Log("Floating label fixture %s tag=%u x=%.1f y=%.1f",names[i],tag,x,y);
    }
}
