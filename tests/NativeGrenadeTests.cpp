#include "../src/platform/NativeGrenades.hpp"
#include <cassert>
#include <cstring>
#include <map>

namespace {
float clockSeconds=0;std::vector<Bounds3> cover;std::vector<wc3::Handle> group;
std::map<wc3::Handle,float> hp;std::map<wc3::Handle,bool> paused;
unsigned damages=0,pauses=0,releases=0,sounds=0;
wc3::Bits Bits(float value){wc3::Bits result;std::memcpy(&result,&value,4);return result;}
void Scene() {clockSeconds=0;cover.clear();hp={{1,1000},{2,1000},{3,1000},{10,1000}};paused={{1,false},{2,false},{3,true}};damages=pauses=releases=sounds=0;}
}
namespace wc3 {
#define NATIVE(result,name,arguments,offset) result (__cdecl* name) arguments=nullptr;
#include "../src/platform/NativeOffsets.inc"
#undef NATIVE
float Real(Bits value){float result;std::memcpy(&result,&value,4);return result;}
float Ground(float,float){return 0;}
float AttackSurface(float,float,bool){return 0;}
void Log(const char*,...) {}
Handle Effect(const char*,float,float){return 42;}
std::vector<Handle> NearbyDestructables(float,float,float){return {10};}
}
void GameAudio::Play(const char*,float){++sounds;}
std::vector<Bounds3> MovementObstacles::Snapshot(wc3::Handle,float,float,float,bool){return cover;}
int main() {
    wc3::CreateTimer=[]()->wc3::Handle{return 1;};wc3::TimerStart=[](wc3::Handle,float*,BOOL,wc3::Handle){};
    wc3::TimerGetElapsed=[](wc3::Handle){return Bits(clockSeconds);};
    wc3::GetUnitTypeId=[](wc3::Handle u){return u && u<10 ? int(100+u) : 0;};
    wc3::GetOwningPlayer=[](wc3::Handle u){return u==1 ? 1u : 2u;};
    wc3::IsUnitAlly=[](wc3::Handle u,wc3::Handle)->BOOL{return u==1;};wc3::IsUnitEnemy=[](wc3::Handle u,wc3::Handle)->BOOL{return u!=1;};
    wc3::GetUnitState=[](wc3::Handle u,int){return Bits(hp[u]);};
    wc3::GetUnitX=[](wc3::Handle u){return Bits(u==1 ? 0.f : 20.f);};wc3::GetUnitY=[](wc3::Handle){return Bits(0);};
    wc3::GetUnitFlyHeight=[](wc3::Handle){return Bits(68);};
    wc3::IsUnitHidden=[](wc3::Handle)->BOOL{return FALSE;};wc3::IsUnitLoaded=wc3::IsUnitHidden;
    wc3::IsUnitType=[](wc3::Handle,int)->BOOL{return FALSE;};
    wc3::IsUnitPaused=[](wc3::Handle u)->BOOL{return paused[u];};
    wc3::PauseUnit=[](wc3::Handle u,BOOL value){paused[u]=value!=FALSE;if(value)++pauses;else ++releases;};
    wc3::CreateGroup=[]()->wc3::Handle{return 1;};wc3::GroupEnumUnitsInRange=[](wc3::Handle,float*,float*,float*,wc3::Handle){group={1,2,3};};
    wc3::FirstOfGroup=[](wc3::Handle)->wc3::Handle{return group.empty() ? 0 : group.front();};
    wc3::GroupRemoveUnit=[](wc3::Handle,wc3::Handle u){group.erase(std::find(group.begin(),group.end(),u));};wc3::DestroyGroup=[](wc3::Handle){};
    wc3::DestroyEffect=[](wc3::Handle){};
    wc3::UnitDamageTarget=[](wc3::Handle,wc3::Handle u,float* damage,BOOL,BOOL,int,int,int)->BOOL{hp[u]-=*damage;++damages;return TRUE;};
    wc3::GetDestructableTypeId=[](wc3::Handle){return 10;};wc3::IsDestructableInvulnerable=[](wc3::Handle)->BOOL{return FALSE;};
    wc3::GetDestructableLife=[](wc3::Handle u){return Bits(hp[u]);};wc3::GetDestructableX=[](wc3::Handle){return Bits(20);};wc3::GetDestructableY=[](wc3::Handle){return Bits(0);};
    GameplaySettings settings;settings.grenades.fuse=.2f;settings.grenades.throwSpeed=50;settings.grenades.gravity=50;settings.friendlyFirePercent=0;
    NativeGrenades field;GameAudio audio;Scene();
    assert(field.Throw(1,WeaponSlots::HE,{0,0,100},{1,0,0},{0,0,0},settings));
    settings.damage[WeaponSlots::HE]=9999; // A later F8 snapshot cannot change the launched charge.
    field.Tick(audio,1,{0,0,100},{1,0,0});assert(damages==0);
    clockSeconds=.3f;assert(field.Tick(audio,1,{0,0,100},{1,0,0}));assert(hp[1]==1000 && hp[2]<1000 && hp[2]>900 && hp[10]<1000);
    unsigned saved=damages;clockSeconds=1;field.Tick(audio,1,{0,0,100},{1,0,0});assert(damages==saved);
    field.Reset();Scene();assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));
    clockSeconds=.3f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(field.FlashOpacity()>0 && paused[2] && paused[3] && !paused[1] && pauses==1 && damages==0);
    field.Tick(audio,1,{0,0,100},{1,0,0});assert(releases==0); // A paused native clock cannot advance a lease.
    clockSeconds=4;field.Tick(audio,1,{0,0,100},{1,0,0});assert(!paused[2] && paused[3] && releases==1 && field.FlashOpacity()==0);
    field.Reset();Scene();settings.grenades.maxActive=1;
    settings.grenades.worldModelScale=1.25f;
    assert(field.Throw(1,WeaponSlots::Smoke,{0,0,100},{1,0,0},{},settings));assert(!field.Throw(1,WeaponSlots::HE,{},{1,0,0},{},settings));
    settings.grenades.worldModelScale=3;clockSeconds=.1f;field.Tick(audio,1,{0,0,100},{1,0,0});
    assert(field.Visuals().size()==1 && field.Visuals()[0].modelScale==1.25f && field.Visuals()[0].spin>0); // Launch snapshot owns world appearance.
    clockSeconds=2;field.Tick(audio,1,{0,0,100},{1,0,0});assert(field.Visuals().size()==1 && field.Visuals()[0].smoke && damages==0);
    clockSeconds=30;field.Tick(audio,1,{0,0,100},{1,0,0});assert(field.Visuals().empty());
    field.Reset();Scene();cover={Bounds3{{12,-30,0},{14,30,200}}};
    assert(field.Throw(1,WeaponSlots::HE,{0,0,100},{1,0,0},{},settings));clockSeconds=.3f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(hp[2]==1000 && hp[10]==1000);
    field.Reset();assert(field.Visuals().empty() && field.FlashOpacity()==0);
    std::puts("PASS native clock, single detonation, damage/friendly/cover/gate policy, F8 snapshots, flash leases and smoke expiry.");
}
