#include "../src/input/FpsTextEntry.hpp"
#include "../src/combat/NuclearInventory.hpp"
#include "../src/config/GameplaySettings.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <windows.h>
namespace wc3 {void Log(const char*,...) {}}
int main(){
    _set_error_mode(_OUT_TO_STDERR);FpsTextEntry text;NuclearInventory bomb;
    text.Open();for(char c:std::string("putinmolodec"))text.Character(c);assert(text.Opened());text.Submit();
    assert(!text.Opened() && text.Transition());assert(FpsTextEntry::NuclearCode(text.Take(),"PUTINMOLODEC"));assert(text.Take().empty());
    assert(!FpsTextEntry::NuclearCode("say PUTINMOLODEC","PUTINMOLODEC"));assert(FpsTextEntry::NuclearCode(" PUTINMOLODEC  ","PUTINMOLODEC"));
    text.Open();text.Character(0x41F);text.Character(0x440);assert(text.Buffer()=="\xD0\x9F\xD1\x80");text.Character(8);assert(text.Buffer()=="\xD0\x9F");text.Cancel();assert(text.Take().empty());
    text.Open();for(int i=0;i<1000;++i)text.Character(0x44F);assert(text.Buffer().size()==240);text.Submit();auto line=text.Take();int chars=0;for(auto p=line.c_str();*p;){assert(UnicodeText::Next(p)==0x44F);++chars;}assert(chars==120);
    text.Open();text.Character('W');text.Submit();text.Reset();assert(text.Take().empty() && !text.Opened());
    assert(!bomb.Grant(false,false));assert(bomb.Grant(true,false) && bomb.Available() && bomb.Selected());assert(bomb.Grant(true,false));bomb.Holster();assert(!bomb.Selected() && bomb.Available());assert(!bomb.Launch());assert(bomb.Grant(true,false));assert(bomb.Launch() && !bomb.Available());assert(!bomb.Grant(true,false));assert(bomb.Grant(true,true));assert(bomb.Launch());bomb.Reset();assert(bomb.Grant(true,false));
    auto path=(std::filesystem::current_path()/"nuclear-validation.ini").string();
    {std::ofstream f(path);f<<"[NuclearBomb]\nEnabled=false\nCode=bad code\nRepeatCodes=true\nDestroyBuildings=true\nKillRewards=false\nDelaySeconds=nan\nRadius=-1\nTargetDistance=inf\nFlashSeconds=99\nShakeDegrees=inf\nEffectSeconds=0\n[Chat]\nEnabled=false\nMessageSeconds=-3\n";}
    auto s=GameplaySettings::Load(path);assert(!s.nuclear.enabled && s.nuclear.code=="PUTINMOLODEC" && s.nuclear.repeatCodes && s.nuclear.buildings && !s.nuclear.rewards);assert(s.nuclear.delay==3 && s.nuclear.targetDistance==800 && s.nuclear.radius==0 && s.nuclear.flashSeconds==3 && s.nuclear.shakeDegrees==.6f && s.nuclear.effectSeconds==3);assert(!s.chatEnabled && s.chatMessageSeconds==0);
    {std::ofstream f(path);f<<"[NuclearBomb]\nCode=MY_CODE-9\nDelaySeconds=0\nRadius=999999\nTargetDistance=-2\nFlashSeconds=0\nShakeDegrees=0\nEffectSeconds=99\n";}
    s=GameplaySettings::Load(path);assert(s.nuclear.code=="MY_CODE-9" && s.nuclear.delay==.5f && s.nuclear.radius==50000 && s.nuclear.targetDistance==100 && s.nuclear.flashSeconds==0 && s.nuclear.shakeDegrees==0 && s.nuclear.effectSeconds==20);DeleteFileA(path.c_str());
    puts("PASS modal UTF-8 input, code matching, charge lifetime, real INI validation and visual opt-outs.");
}
