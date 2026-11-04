#include "PortalsHeavyScene.hpp"
#include "BackgroundGameplay.hpp"
#include "../src/platform/GameBuild.hpp"
#include "../src/platform/NativeRegistry.hpp"
#include <fstream>
#include <vector>
#include <string>
namespace {
std::vector<wc3::Handle> targets;DWORD until=0,last=0;
void Snapshot() {
    if(!targets.empty())return;
    auto group=wc3::CreateGroup();wc3::GroupEnumUnitsOfPlayer(group,wc3::Player(1),0);
    for(auto unit=wc3::FirstOfGroup(group);unit;unit=wc3::FirstOfGroup(group)) {wc3::GroupRemoveUnit(group,unit);targets.push_back(unit);}
    wc3::DestroyGroup(group);
}
void Inspect(ShooterController& controller,uintptr_t base) {
    controller.TestHeavyState();using Data=int (__cdecl*)(wc3::Handle);
    static auto data=reinterpret_cast<Data>(NativeRegistry::Resolve(base,"GetUnitUserData","(Hunit;)I"));
    auto actor=controller.TestUnit();if(actor && data)wc3::Log("fixture heavy deaths=%d",data(actor));
    for(auto target:targets) {
        int type=wc3::GetUnitTypeId(target);auto object=type ? wc3::UnitObject(target) : 0;uint32_t flags=0;SIZE_T got=0;
        // Explicit 1.26a fixture: no raw model pointers survive native damage callbacks.
        if(object)ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(object+0x5C),&flags,4,&got);
        wc3::Log("fixture heavy target=%08X type=%08X hp=%.2f flags=%08X",target,type,type ? wc3::Real(wc3::GetUnitState(target,0)) : 0,flags);
    }
}
}
void PortalsHeavyScene::Tick(ShooterController& controller,const char* root,uintptr_t base) {
    if(!BackgroundGameplay::Enabled() || wc3::ActiveBuild()!=wc3::GameBuildKind::Classic126a)return;
    auto path=std::string(root)+"\\test-portals-heavy.request";
    if(GetFileAttributesA(path.c_str())!=INVALID_FILE_ATTRIBUTES) {
        std::ifstream input(path);std::string command;input>>command;Snapshot();
        if(command=="inspect")Inspect(controller,base);
        else if(command=="watch") {DWORD ms=0;input>>ms;until=GetTickCount()+std::min(ms,DWORD(30000));}
        else if(command=="aim") {
            std::string raw;int head=0;input>>raw>>head;
            if(raw.size()==4) {
                int type=(raw[0]<<24)|(raw[1]<<16)|(raw[2]<<8)|raw[3];
                for(auto target:targets)if(wc3::GetUnitTypeId(target)==type && wc3::Real(wc3::GetUnitState(target,0))>.405f) {
                    if(!head || !controller.TestAimHead(target)) {
                        float x=wc3::Real(wc3::GetUnitX(target)),y=wc3::Real(wc3::GetUnitY(target));
                        controller.TestAimPoint(x,y,wc3::Ground(x,y)+wc3::Real(wc3::GetUnitFlyHeight(target))+50);
                    }
                    wc3::Log("fixture heavy aim target=%08X type=%08X",target,type);break;
                }
            }
        }
        input.close();DeleteFileA(path.c_str());
    }
    if(controller.Visible() && GetTickCount()<until && GetTickCount()-last>=100){last=GetTickCount();Inspect(controller,base);}
}
