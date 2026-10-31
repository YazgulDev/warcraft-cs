#include "../src/platform/NativeNuclearBomb.hpp"
#include <cassert>
#include <map>
#include <vector>
#include <cstring>
#include <cstdio>
#include <algorithm>
namespace {
float clockSeconds=0;unsigned sounds=0,damages=0,destroyed=0,rewardCount=0,forced=0,props=0;bool archiveReady=true;std::string lastSound;float lastGain=0;bool single=true,actorPresent=true,transfer=false;
std::map<wc3::Handle,float> health;std::vector<wc3::Handle> group;NativeNuclearBomb* resetOnDamage=nullptr;
NativeRockets::ImpactSink payload;std::vector<std::string> effectModels;
wc3::Bits Bits(float x){wc3::Bits b;std::memcpy(&b,&x,4);return b;}
void Scene(){clockSeconds=0;sounds=damages=destroyed=rewardCount=forced=props=0;single=actorPresent=true;transfer=false;resetOnDamage=nullptr;payload={};archiveReady=true;lastSound.clear();lastGain=0;effectModels.clear();health.clear();for(int i=1;i<=23;++i)health[i]=1000;}
}
namespace wc3 {
#define NATIVE(result,name,arguments,offset) result (__cdecl* name) arguments=nullptr;
#include "../src/platform/NativeOffsets.inc"
#undef NATIVE
float Real(Bits bits){float x;std::memcpy(&x,&bits,4);return x;}
void Log(const char*,...){}void LogError(const char*,...){}
bool SinglePlayer(){return single;}
float Ground(float,float){return 0;}
uintptr_t UnitObject(Handle unit){return unit==1 && !actorPresent ? 0 : unit+0x1000;}
uintptr_t DestructableObject(Handle unit){return unit+0x2000;}
bool IsNeutralUnit(Handle unit){return unit==5 || unit==9 || unit==12;}
std::vector<Handle> NearbyDestructables(float,float,float){return {21,22,23};}
void KillDestructable(Handle unit){health[unit]=0;++props;}
}
void GameAudio::Play(const char* name,float gain){++sounds;lastSound=name;lastGain=gain;}
bool GameAudio::PlayArchive(const char* path,float gain){if(!archiveReady)return false;++sounds;lastSound=path;lastGain=gain;return true;}
bool NativeRockets::Launch(wc3::Handle,const GrenadePoint&,const GrenadePoint&,const GrenadePoint&,const GameplaySettings&,ImpactSink sink){payload=std::move(sink);return true;}
void NativeEffectService::Configure(uintptr_t){}
bool NativeEffectService::Spawn(const char* model,const GrenadePoint&,float,float,bool){effectModels.push_back(model);return true;}
void NativeEffectService::Tick(float){}
int main(){
    _set_error_mode(_OUT_TO_STDERR);
    wc3::CreateTimer=[]()->wc3::Handle{return 90;};wc3::TimerStart=[](wc3::Handle,float*,BOOL,wc3::Handle){};
    wc3::TimerGetElapsed=[](wc3::Handle){return Bits(clockSeconds);};wc3::DestroyTimer=[](wc3::Handle){++destroyed;};
    wc3::CreateGroup=[]()->wc3::Handle{return 80;};wc3::DestroyGroup=[](wc3::Handle){};
    wc3::GroupEnumUnitsInRange=[](wc3::Handle,float*,float*,float*,wc3::Handle){group.clear();for(unsigned i=1;i<=12;++i)group.push_back(i);};
    wc3::FirstOfGroup=[](wc3::Handle)->wc3::Handle{return group.empty() ? 0 : group.front();};wc3::GroupRemoveUnit=[](wc3::Handle,wc3::Handle unit){group.erase(std::find(group.begin(),group.end(),unit));};
    wc3::GetUnitTypeId=[](wc3::Handle u){return u==1 && !actorPresent ? 0 : int(u+200);};
    wc3::GetOwningPlayer=[](wc3::Handle u)->wc3::Handle{return u==1 ? (transfer ? 101u : 100u) : 112u;};
    wc3::GetUnitState=[](wc3::Handle u,int state){return Bits(state==1 ? 1000 : health[u]);};
    wc3::IsUnitEnemy=[](wc3::Handle u,wc3::Handle)->BOOL{return u!=1 && u!=4 && u!=5 && u!=9;};wc3::IsUnitAlly=[](wc3::Handle u,wc3::Handle)->BOOL{return u==1 || u==4 || u==9;};
    wc3::IsUnitHidden=[](wc3::Handle u)->BOOL{return u==6;};wc3::IsUnitLoaded=[](wc3::Handle u)->BOOL{return u==7;};
    wc3::IsUnitType=[](wc3::Handle u,int type)->BOOL{return (u==9 || u==11 || u==12) && type==2;};wc3::GetUnitAbilityLevel=[](wc3::Handle u,int){return u==8 || u==9 ? 1 : 0;};
    wc3::GetUnitX=[](wc3::Handle u){return Bits(u==10 || u==12 ? 16000.f : (u==3 ? 6000.f : 50.f));};wc3::GetUnitY=[](wc3::Handle){return Bits(0);};
    wc3::GetDestructableTypeId=[](wc3::Handle u){return int(u+300);};wc3::GetDestructableLife=[](wc3::Handle u){return Bits(health[u]);};
    wc3::GetDestructableX=[](wc3::Handle u){return Bits(u==23 ? 16000.f : 50.f);};wc3::GetDestructableY=[](wc3::Handle){return Bits(0);};
    wc3::KillUnit=[](wc3::Handle u){health[u]=0;++forced;};
    wc3::UnitDamageTarget=[](wc3::Handle actor,wc3::Handle target,float* damage,BOOL attack,BOOL ranged,int attackType,int damageType,int)->BOOL{
        assert(actor==1 && !attack && ranged && attackType==5 && damageType==26);++damages;if(target!=9)health[target]-=*damage;
        if(resetOnDamage)resetOnDamage->Reset();return TRUE;
    };
    GameAudio audio;NativeNuclearBomb bomb;NativeRockets rockets;GameplaySettings settings;auto sink=[](const KillReward& reward){assert(reward.owner==100);++rewardCount;};
    Scene();assert(bomb.Launch(1,{0,0,96},{1,0,0},{},settings,rockets));assert(bomb.Flying() && payload);assert(!bomb.Launch(1,{0,0,96},{1,0,0},{},settings,rockets));clockSeconds=5;bomb.Tick(audio,sink);assert(!damages && effectModels.empty());
    payload({0,0,0});bomb.Tick(audio,sink);assert(!bomb.Flying() && damages==4 && rewardCount==2 && forced==1 && props==2);
    assert(sounds==1 && lastSound.find("SapperBlowsUp.wav")!=std::string::npos && lastGain==1);
    assert(health[2]<0 && health[3]<0 && health[5]<0 && health[9]==0);for(int i:{1,4,6,7,8,10,11,12,23})assert(health[i]==1000);
    assert(effectModels.size()==1 && effectModels[0].find("NeutralBuildingExplosion")!=std::string::npos);assert(bomb.Exposure({30000,0,0})==0 && bomb.Exposure({0,0,0})==1);
    clockSeconds=5.9f;bomb.Tick(audio,sink);assert(effectModels.size()==9 && effectModels[1].find("FlameStrike1")!=std::string::npos);
    clockSeconds=13;bomb.Tick(audio,sink);assert(effectModels.size()==46 && effectModels.back().find("SmokeSmudge0")!=std::string::npos);
    clockSeconds=32;bomb.Tick(audio,sink);assert(!bomb.Active() && destroyed==1);
    // Disable neutral/terrain policy independently; launch snapshots ignore later F8 changes.
    bomb.Reset();Scene();settings.nuclear.neutralBuildings=settings.nuclear.neutralUnits=settings.nuclear.destructables=false;settings.nuclear.rewards=false;settings.nuclear.flashSeconds=settings.nuclear.shakeDegrees=0;
    assert(bomb.Launch(1,{0,0,96},{1,0,0},{},settings,rockets));settings.nuclear.neutralBuildings=true;payload({0,0,0});bomb.Tick(audio,sink);
    assert(health[5]==1000 && health[9]==1000 && props==0 && forced==0 && rewardCount==0 && bomb.Flash()==0 && bomb.Shake()==0);
    bomb.Reset();Scene();assert(bomb.Launch(1,{0,0,96},{1,0,0},{},settings,rockets));transfer=true;payload({0,0,0});bomb.Tick(audio,sink);assert(!damages);
    // Native death callbacks can unload a map; neither later victims nor stale effects/timers may be touched.
    bomb.Reset();Scene();assert(bomb.Launch(1,{0,0,96},{1,0,0},{},settings,rockets));resetOnDamage=&bomb;payload({0,0,0});bomb.Tick(audio,sink);assert(damages==1 && !bomb.Active() && destroyed==0);
    // Explicit mute and archive failure cannot replay the old debris sound; missing owned audio uses a real C4 blast.
    bomb.Reset();Scene();settings.nuclear.explosionSound="off";
    assert(bomb.Launch(1,{0,0,96},{1,0,0},{},settings,rockets));payload({0,0,0});bomb.Tick(audio,sink);assert(sounds==0);
    bomb.Reset();Scene();settings.nuclear.explosionSound="city";settings.nuclear.explosionVolumePercent=35;archiveReady=false;
    assert(bomb.Launch(1,{0,0,96},{1,0,0},{},settings,rockets));payload({0,0,0});bomb.Tick(audio,sink);assert(sounds==1 && lastSound=="c4_explode1.wav" && std::abs(lastGain-.35f)<.001f);
    bomb.Reset();Scene();single=false;assert(!bomb.Launch(1,{0,0,96},{1,0,0},{},settings,rockets));
    puts("PASS nuclear RPG payload/impact sequence, radial filtering, neutral invulnerable shop native death, scenery path, snapshots, protected/distant allies, local flash, callback map reset and expiry.");
}
