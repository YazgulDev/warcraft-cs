#include "../src/config/SettingsMenu.hpp"
#include "../src/platform/SettingsStore.hpp"
#include "../src/presentation/SettingsMenuLayout.hpp"
#include "../src/input/LookAngles.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <limits>
#include <cstdlib>
namespace wc3 {void Log(const char*,...) {}}
int main() {
    _set_error_mode(_OUT_TO_STDERR);using K=SettingsMenu::Kind;using R=SettingsMenu;
    GameplaySettings s;SettingsMenu menu;std::vector<ConfigChange> written;int writes=0;
    auto skies=[] {return std::vector<std::string>{"Des","forest","snow"};};
    auto save=[&](const auto& changes){written=changes;++writes;return true;};
    auto update=[&]{return menu.Update(s,'A',skies,save);};
    menu.Queue({K::Toggle});assert(menu.RoutingOpen() && !menu.Open());update();assert(menu.Open() && !menu.Dirty());
    menu.Queue({K::Adjust,R::Sensitivity,5});update();assert(menu.Dirty() && s.mouse.sensitivity==1);
    menu.Queue({K::Cancel});update();assert(!menu.Open() && !writes);
    menu.Queue({K::Toggle});update();assert(!menu.Dirty() && menu.Value(R::Sensitivity)=="1x");
    menu.Queue({K::Adjust,R::Sensitivity,1000});menu.Queue({K::Adjust,R::Volume,-1000});menu.Queue({K::Adjust,R::Physics,1});
    menu.Queue({K::Slide,R::ViewDistance,1});menu.Queue({K::Adjust,R::SkyPreset,1});menu.Queue({K::Apply});
    assert(update() && !menu.Open() && writes==1 && written.size()==5);
    assert(written[0].section=="Mouse" && written[0].value=="5");
    assert(written[1].section=="Audio" && written[1].value=="0");
    assert(written[2].key=="Mode" && written[2].value=="recovery");assert(written.back().section=="Sky" && written.back().key=="A");
    menu.Queue({K::Toggle});update();menu.Queue({K::Adjust,R::SkyVisible,-1});menu.Queue({K::Apply});
    assert(!menu.Update(s,'A',skies,[](const auto&){return false;}) && menu.Open() && !menu.Error().empty());
    menu.Queue({K::Apply});assert(update() && !menu.Open());
    menu.Queue({K::Toggle});update();menu.Queue({K::Slide,R::Sensitivity,std::numeric_limits<float>::quiet_NaN()});update();assert(!menu.Dirty());
    menu.Queue({K::Adjust,R::ViewDistance,-100});update();menu.Queue({K::Defaults});update();assert(!menu.Dirty());
    menu.Queue({K::Adjust,R::Volume,-1});menu.Reset();assert(!menu.RoutingOpen()); // Map/focus cancellation forgets queued edits.
    for(int i=0;i<1000;++i)menu.Queue({K::Toggle});menu.Reset();assert(!menu.RoutingOpen());
    // No-CS clients retain native controls and cannot select an empty CS sky source.
    menu.Queue({K::Toggle});menu.Update(s,0,[]{return std::vector<std::string>{};},save);
    menu.Queue({K::Adjust,R::SkyMode,1});menu.Queue({K::Adjust,R::SkyPreset,1});menu.Update(s,0,skies,save);
    assert(menu.Value(R::SkyMode)=="Warcraft" && !menu.Dirty());menu.Reset();
    // Main and scoped sensitivity use the same gain that the real movement/camera path consumes.
    assert(s.mouse.Gain(0)==.14f && s.mouse.Gain(1)==.035f && s.mouse.Gain(2)==.009f);
    s.mouse.sensitivity=2;s.mouse.scopeSensitivity=.5f;assert(s.mouse.Gain(0)==.28f && s.mouse.Gain(1)==.035f);
    float yaw=90,pitch=0;LookAngles::Apply(yaw,pitch,100,10,s.mouse.Gain(0));assert(std::abs(yaw-62)<.001f && std::abs(pitch+2.8f)<.001f);
    for(auto size:{std::pair<int,int>{800,600},{1024,768},{1920,1080},{2560,1080}}) {
        SettingsMenuLayout box(size.first,size.second);
        for(int row=0;row<R::Rows;++row) {
            auto hit=box.Test(int(box.plusX+1),int(box.rowY+(row+.4f)*box.row));assert(hit.kind==SettingsMenuLayout::HitKind::Plus && hit.row==row);
            assert(box.rowY+(row+.88f)*box.row<box.actionY);
        }
        assert(box.Test(int(box.x+box.width*.1f),int(box.actionY+2)).kind==SettingsMenuLayout::HitKind::Apply);
        assert(box.Test(0,0).kind==SettingsMenuLayout::HitKind::None);
    }
    // Saving is atomic, preserves custom/unrelated keys, and the real loader sees each subsequent edit.
    auto build=std::filesystem::current_path();assert(build.filename()=="build");
    auto folder=build/("settings-menu-test-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount()));
    assert(folder.lexically_normal().parent_path()==build && std::filesystem::create_directory(folder));
    auto path=(folder/"WarcraftCS.ini").string();
    {std::ofstream file(path);file<<"; keep comment\n[Unrelated]\nPrivate=keep-this\n[Mouse]\nSensitivity=1\n[Movement]\nMode=classic\n[Sky]\nA=forest\n";}
    assert(SettingsStore::Save(path,{{"Mouse","Sensitivity","2.5"},{"Audio","CSVolumePercent","45"},{"Movement","Mode","recovery"},{"Sky","Visible","false"},{"Sky","A","snow"}}));
    auto loaded=GameplaySettings::Load(path);assert(loaded.mouse.sensitivity==2.5f && loaded.csVolumePercent==45 && !loaded.movement.classicCollision && loaded.movement.platformRecovery && !loaded.skyVisible && loaded.tilesetSky['A']=="snow");
    assert(SettingsStore::Save(path,{{"Mouse","Sensitivity","0.75"}}));assert(GameplaySettings::Load(path).mouse.sensitivity==.75f);
    char keep[32]={};GetPrivateProfileStringA("Unrelated","Private","",keep,32,path.c_str());assert(std::string(keep)=="keep-this");
    std::ifstream file(path);std::string content((std::istreambuf_iterator<char>(file)),{});file.close();assert(content.find("; keep comment")!=std::string::npos);
    assert(!SettingsStore::Save(path,{{"Other","Unsafe","value"}}));
    assert(!SettingsStore::Save(path,{{"Sky","A","../bad"}}));
    assert(SetFileAttributesA(path.c_str(),FILE_ATTRIBUTE_READONLY));assert(!SettingsStore::Save(path,{{"Mouse","Sensitivity","4"}}));
    assert(GameplaySettings::Load(path).mouse.sensitivity==.75f);SetFileAttributesA(path.c_str(),FILE_ATTRIBUTE_NORMAL);
    auto assets=folder/"assets/skies";std::filesystem::create_directories(assets);
    auto cache=[&](const char* name,bool full){std::ofstream out(assets/name,std::ios::binary);uint32_t one=1;out.write("WCS1",4);out.write(reinterpret_cast<char*>(&one),4);out.write(reinterpret_cast<char*>(&one),4);if(full){char data[24]={};out.write(data,24);}};
    cache("snow.wcs",true);cache("bad.wcs",false);assert(SettingsStore::Skies(folder.string())==std::vector<std::string>{"snow"});
    assert(std::filesystem::canonical(folder).parent_path()==std::filesystem::canonical(build));
    std::filesystem::remove_all(folder); // Remove only the unique directory created and verified by this test.
    std::puts("PASS menu drafts/cancel/save failure, modal queue, shared sensitivity, bounded layout, atomic INI, live reload and private sky validation.");
}
