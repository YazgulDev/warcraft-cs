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
    {std::ofstream f(path);f<<"[NuclearBomb]\nEnabled=false\nCode=bad code\nRepeatCodes=true\nDestroyBuildings=true\nKillRewards=false\nRadius=-1\nExplosionScale=inf\nFlameScale=0\nSmokeSeconds=nan\nFlashSeconds=99\nShakeDegrees=inf\nScreenEffectRadius=0\n[Chat]\nEnabled=false\nMessageSeconds=-3\n";}
    auto s=GameplaySettings::Load(path);assert(!s.nuclear.enabled && s.nuclear.code=="PUTINMOLODEC" && s.nuclear.repeatCodes && s.nuclear.buildings && !s.nuclear.rewards);
    assert(s.nuclear.radius==100 && s.nuclear.explosionScale==12 && s.nuclear.flameScale==.25f && s.nuclear.smokeSeconds==18 && s.nuclear.flashSeconds==3 && s.nuclear.shakeDegrees==.6f && s.nuclear.screenRadius==0);assert(!s.chatEnabled && s.chatMessageSeconds==0);
    {std::ofstream f(path);f<<"[NuclearBomb]\nCode=MY_CODE-9\nRadius=999999\nFlashSeconds=0\nShakeDegrees=0\nFlameDelaySeconds=999\nFlameSeconds=999\nSmokeSeconds=999\nDestroyNeutralBuildings=false\nDestroyDestructables=false\n";}
    s=GameplaySettings::Load(path);assert(s.nuclear.code=="MY_CODE-9" && s.nuclear.radius==20000 && s.nuclear.flameDelay==5 && s.nuclear.flameSeconds==15 && s.nuclear.smokeSeconds==45 && !s.nuclear.neutralBuildings && !s.nuclear.destructables);
    {std::ofstream f(path);f<<"[NuclearBomb]\nRadius=0\n";}assert(GameplaySettings::Load(path).nuclear.radius==8000);
    {std::ofstream f(path);f<<"[NuclearBomb]\nBehaviorVersion=2\nRadius=0\n";}assert(GameplaySettings::Load(path).nuclear.radius==100);
    // Stock tuning upgrades once; explicit v3 opt-backs and custom density/sound settings survive.
    {std::ofstream f(path);f<<"[NuclearBomb]\nBehaviorVersion=2\nRadius=1600\nSmokeSeconds=8\nSmokeScale=6\n";}s=GameplaySettings::Load(path);assert(s.nuclear.radius==8000 && s.nuclear.smokeSeconds==18 && s.nuclear.smokeScale==14 && s.nuclear.smokeEmitters==37);
    {std::ofstream f(path);f<<"[NuclearBomb]\nBehaviorVersion=3\nRadius=1600\nSmokeSeconds=8\nSmokeScale=6\nSmokeEmitters=999\nExplosionSound=CiTy\nExplosionVolumePercent=999\n";}s=GameplaySettings::Load(path);assert(s.nuclear.radius==1600 && s.nuclear.smokeSeconds==8 && s.nuclear.smokeScale==6 && s.nuclear.smokeEmitters==49 && s.nuclear.explosionSound=="city" && s.nuclear.explosionVolumePercent==100);
    {std::ofstream f(path);f<<"[NuclearBomb]\nSmokeEmitters=-1\nExplosionSound=unknown\nExplosionVolumePercent=nan\n";}s=GameplaySettings::Load(path);assert(s.nuclear.smokeEmitters==1 && s.nuclear.explosionSound=="sapper" && s.nuclear.explosionVolumePercent==100);
    // Building policy migrates independently from balance, and deliberate versioned opt-outs remain selectable.
    {std::ofstream f(path);f<<"[NuclearBomb]\nDestroyBuildings=false\n";}s=GameplaySettings::Load(path);assert(s.nuclear.buildings && s.nuclear.forceEnemyDeath);
    {std::ofstream f(path);f<<"[NuclearBomb]\nDestructionVersion=2\nDestroyBuildings=false\nForceEnemyDeath=false\n";}s=GameplaySettings::Load(path);assert(!s.nuclear.buildings && !s.nuclear.forceEnemyDeath);
    DeleteFileA(path.c_str());
    puts("PASS modal UTF-8 input, code matching, charge lifetime, real INI validation and visual opt-outs.");
}
