#include "../src/platform/NativeNuclearBomb.hpp"
#include <cassert>
#include <map>
#include <vector>
#include <cstring>
#include <cstdio>
#include <algorithm>
namespace {
float clockSeconds=0;unsigned sounds=0,damages=0,destroyed=0,rewardCount=0;bool single=true,actorPresent=true,transfer=false;
std::map<wc3::Handle,float> health;std::vector<wc3::Handle> group;NativeNuclearBomb* resetOnDamage=nullptr;
wc3::Bits Bits(float x){wc3::Bits b;std::memcpy(&b,&x,4);return b;}
void Scene(){clockSeconds=0;sounds=damages=destroyed=rewardCount=0;single=actorPresent=true;transfer=false;resetOnDamage=nullptr;health.clear();for(int i=1;i<=10;++i)health[i]=1000;}
}
namespace wc3 {
#define NATIVE(result,name,arguments,offset) result (__cdecl* name) arguments=nullptr;
#include "../src/platform/NativeOffsets.inc"
#undef NATIVE
float Real(Bits bits){float x;std::memcpy(&x,&bits,4);return x;}
void Log(const char*,...){}void LogError(const char*,...){}
bool SinglePlayer(){return single;}
uintptr_t UnitObject(Handle unit){return unit==1 && !actorPresent ? 0 : unit+0x1000;}
Handle Effect(const char*,float,float){return 50;}
}
void GameAudio::Play(const char*,float){++sounds;}
int main(){
    _set_error_mode(_OUT_TO_STDERR);
    wc3::CreateTimer=[]()->wc3::Handle{return 90;};wc3::TimerStart=[](wc3::Handle,float*,BOOL,wc3::Handle){};
    wc3::TimerGetElapsed=[](wc3::Handle){return Bits(clockSeconds);};wc3::DestroyTimer=[](wc3::Handle){++destroyed;};
    wc3::Player=[](int slot)->wc3::Handle{return unsigned(slot+100);};wc3::CreateGroup=[]()->wc3::Handle{return 80;};wc3::DestroyGroup=[](wc3::Handle){};
    wc3::GroupEnumUnitsOfPlayer=[](wc3::Handle,wc3::Handle owner,wc3::Handle){group.clear();if(owner==112)for(unsigned i=1;i<=10;++i)group.push_back(i);};
    wc3::FirstOfGroup=[](wc3::Handle)->wc3::Handle{return group.empty() ? 0 : group.front();};wc3::GroupRemoveUnit=[](wc3::Handle,wc3::Handle unit){group.erase(std::find(group.begin(),group.end(),unit));};
    wc3::GetUnitTypeId=[](wc3::Handle u){return u==1 && !actorPresent ? 0 : int(u+200);};
    wc3::GetOwningPlayer=[](wc3::Handle u)->wc3::Handle{return u==1 ? (transfer ? 101u : 100u) : 112u;};
    wc3::GetUnitState=[](wc3::Handle u,int state){return Bits(state==1 ? 1000 : health[u]);};
    wc3::IsUnitEnemy=[](wc3::Handle u,wc3::Handle)->BOOL{return u!=1 && u!=4 && u!=5;};wc3::IsUnitAlly=[](wc3::Handle u,wc3::Handle)->BOOL{return u==1 || u==4;};
    wc3::IsUnitHidden=[](wc3::Handle u)->BOOL{return u==6;};wc3::IsUnitLoaded=[](wc3::Handle u)->BOOL{return u==7;};
    wc3::IsUnitType=[](wc3::Handle u,int type)->BOOL{return u==9 && type==2;};wc3::GetUnitAbilityLevel=[](wc3::Handle u,int){return u==8 ? 1 : 0;};
    wc3::GetUnitX=[](wc3::Handle u){return Bits(u==10 ? 8000.f : 50.f);};wc3::GetUnitY=[](wc3::Handle){return Bits(0);};wc3::DestroyEffect=[](wc3::Handle){};
    wc3::UnitDamageTarget=[](wc3::Handle actor,wc3::Handle target,float* damage,BOOL attack,BOOL ranged,int attackType,int damageType,int)->BOOL{
        assert(actor==1 && !attack && ranged && attackType==5 && damageType==26);++damages;health[target]-=*damage;
        if(resetOnDamage)resetOnDamage->Reset();return TRUE;
    };
    GameAudio audio;NativeNuclearBomb bomb;NuclearSettings settings;auto sink=[](const KillReward& reward){assert(reward.owner==100);++rewardCount;};
    Scene();assert(bomb.Launch(1,{0,0,0},settings));assert(!bomb.Launch(1,{0,0,0},settings));clockSeconds=2.99f;bomb.Tick(audio,sink);assert(damages==0);
    clockSeconds=3;bomb.Tick(audio,sink);assert(damages==3 && rewardCount==3);assert(health[2]<0 && health[3]<0 && health[10]<0);for(int i:{1,4,5,6,7,8,9})assert(health[i]==1000);
    auto count=damages;bomb.Tick(audio,sink);assert(damages==count && bomb.Flash()>0);clockSeconds=12;bomb.Tick(audio,sink);assert(!bomb.Active() && destroyed==1);
    bomb.Reset();Scene();settings.radius=100;settings.buildings=true;settings.rewards=false;settings.flashSeconds=settings.shakeDegrees=0;
    assert(bomb.Launch(1,{0,0,0},settings));settings.radius=0;settings.buildings=false;clockSeconds=3;bomb.Tick(audio,sink);
    assert(health[9]<0 && health[10]==1000 && damages==3 && rewardCount==0 && bomb.Flash()==0 && bomb.Shake()==0);
    bomb.Reset();Scene();settings.radius=0;assert(bomb.Launch(1,{0,0,0},settings));transfer=true;clockSeconds=3;bomb.Tick(audio,sink);assert(!damages);
    bomb.Reset();Scene();assert(bomb.Launch(1,{0,0,0},settings));actorPresent=false;clockSeconds=3;bomb.Tick(audio,sink);assert(!damages);
    // A native death callback changing maps must stop this old snapshot before another victim is touched.
    bomb.Reset();Scene();assert(bomb.Launch(1,{0,0,0},settings));resetOnDamage=&bomb;clockSeconds=3;bomb.Tick(audio,sink);assert(damages==1 && !bomb.Active() && destroyed==0);
    bomb.Reset();Scene();single=false;assert(!bomb.Launch(1,{0,0,0},settings));single=true;settings.enabled=false;assert(!bomb.Launch(1,{0,0,0},settings));
    puts("PASS native nuclear pause clock, map-wide/radius filtering, invulnerability/allies, launch snapshots, rewards, callback map reset and actor identity.");
}
