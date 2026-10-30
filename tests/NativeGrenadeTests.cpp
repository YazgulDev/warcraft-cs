#include "../src/platform/NativeRockets.hpp"
#include "../src/platform/NativeGrenades.hpp"
#include <cassert>
#include "../src/platform/SpriteTransform.hpp"
#include <cstring>
#include <map>
#include <cstdlib>

namespace {
float clockSeconds=0;std::vector<Bounds3> cover;std::vector<wc3::Handle> group;
std::map<wc3::Handle,float> hp;std::map<wc3::Handle,bool> paused;
unsigned damages=0,pauses=0,releases=0,sounds=0;
std::vector<bool> collisionFilters;
std::vector<wc3::Handle> enumerated={1,2,3};bool actorPresent=true;
wc3::Bits Bits(float value){wc3::Bits result;std::memcpy(&result,&value,4);return result;}
void Scene() {clockSeconds=0;cover.clear();hp={{1,1000},{2,1000},{3,1000},{10,1000}};paused={{1,false},{2,false},{3,true}};damages=pauses=releases=sounds=0;enumerated={1,2,3};actorPresent=true;}
}
namespace wc3 {
#define NATIVE(result,name,arguments,offset) result (__cdecl* name) arguments=nullptr;
#include "../src/platform/NativeOffsets.inc"
#undef NATIVE
float Real(Bits value){float result;std::memcpy(&result,&value,4);return result;}
float Ground(float,float){return 0;}
float AttackSurface(float,float,bool){return 0;}
void Log(const char*,...) {}
void Trace(const char*,...) {}
Handle Effect(const char*,float,float){return 42;}
std::vector<Handle> NearbyDestructables(float,float,float){return {10};}
}
namespace SpriteTransform {
bool Read(uintptr_t sprite,ModelTransform& transform){transform.position[0]=sprite==1 ? -400.f : 400.f;return sprite!=0;}
}
void GameAudio::Play(const char*,float){++sounds;}
void GameAudio::SetLoop(const char*,bool,float){}
// Combat must retain coarse cover snapshots when movement opts into static mesh detail.
std::vector<Bounds3> MovementObstacles::Snapshot(wc3::Handle,float,float,float,bool nativeCollision,bool detailed,bool){assert(!detailed);collisionFilters.push_back(nativeCollision);return cover;}
int main() {
    _set_error_mode(_OUT_TO_STDERR);
    wc3::CreateTimer=[]()->wc3::Handle{return 1;};wc3::TimerStart=[](wc3::Handle,float*,BOOL,wc3::Handle){};
    wc3::TimerGetElapsed=[](wc3::Handle){return Bits(clockSeconds);};
    wc3::GetUnitTypeId=[](wc3::Handle u){return u && u<10 && (u!=1 || actorPresent) ? int(100+u) : 0;};
    wc3::GetOwningPlayer=[](wc3::Handle u){return u==1 ? 1u : 2u;};
    wc3::IsUnitAlly=[](wc3::Handle u,wc3::Handle)->BOOL{return u==1 || u==4;};wc3::IsUnitEnemy=[](wc3::Handle u,wc3::Handle)->BOOL{return u!=1 && u!=4 && u!=5;};
    wc3::GetUnitState=[](wc3::Handle u,int){return Bits(hp[u]);};
    wc3::GetUnitX=[](wc3::Handle u){return Bits(u==1 ? 0.f : 20.f);};wc3::GetUnitY=[](wc3::Handle){return Bits(0);};
    wc3::GetUnitFlyHeight=[](wc3::Handle){return Bits(68);};
    wc3::IsUnitHidden=[](wc3::Handle u)->BOOL{return u==7;};wc3::IsUnitLoaded=[](wc3::Handle u)->BOOL{return u==8;};
    wc3::IsUnitType=[](wc3::Handle u,int type)->BOOL{return u==9 && type==2;};
    wc3::IsUnitPaused=[](wc3::Handle u)->BOOL{return paused[u];};
    wc3::PauseUnit=[](wc3::Handle u,BOOL value){paused[u]=value!=FALSE;if(value)++pauses;else ++releases;};
    wc3::CreateGroup=[]()->wc3::Handle{return 1;};wc3::GroupEnumUnitsInRange=[](wc3::Handle,float*,float*,float*,wc3::Handle){group=enumerated;};
    wc3::FirstOfGroup=[](wc3::Handle)->wc3::Handle{return group.empty() ? 0 : group.front();};
    wc3::GroupRemoveUnit=[](wc3::Handle,wc3::Handle u){group.erase(std::find(group.begin(),group.end(),u));};wc3::DestroyGroup=[](wc3::Handle){};
    wc3::DestroyEffect=[](wc3::Handle){};
    wc3::UnitDamageTarget=[](wc3::Handle,wc3::Handle u,float* damage,BOOL,BOOL,int,int,int)->BOOL{hp[u]-=*damage;++damages;return TRUE;};
    wc3::GetDestructableTypeId=[](wc3::Handle){return 10;};wc3::IsDestructableInvulnerable=[](wc3::Handle)->BOOL{return FALSE;};
    wc3::GetDestructableLife=[](wc3::Handle u){return Bits(hp[u]);};wc3::GetDestructableX=[](wc3::Handle){return Bits(20);};wc3::GetDestructableY=[](wc3::Handle){return Bits(0);};
    GameplaySettings settings;settings.grenades.fuse=.2f;settings.grenades.throwSpeed=50;settings.grenades.gravity=50;settings.friendlyFirePercent=0;
    NativeGrenades field;GameAudio audio;Scene();
    // The production HE/blast path reports each killed hostile once, including delayed detonations.
    int rewards=0;field.SetKillSink([&](const KillReward& reward){assert(reward.owner==1);++rewards;});
    assert(field.Throw(1,WeaponSlots::HE,{0,0,100},{1,0,0},{0,0,0},settings));
    settings.damage[WeaponSlots::HE]=9999; // A later F8 snapshot cannot change the launched charge.
    field.Tick(audio,1,{0,0,100},{1,0,0});assert(damages==0);
    clockSeconds=.3f;assert(field.Tick(audio,1,{0,0,100},{1,0,0}));assert(hp[1]==1000 && hp[2]<1000 && hp[2]>400 && hp[2]<800 && hp[10]<1000);
    unsigned saved=damages;clockSeconds=1;field.Tick(audio,1,{0,0,100},{1,0,0});assert(damages==saved);
    field.Reset();Scene();assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));
    // F8 changes after release cannot rewrite the already flying grenade's eligibility/duration snapshot.
    settings.grenades.flashStunAllUnits=false;settings.grenades.flashStunDistanceFalloff=true;
    clockSeconds=.3f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(field.FlashOpacity()>0 && paused[2] && paused[3] && paused[1] && pauses==2 && damages==0);
    field.Tick(audio,1,{0,0,100},{1,0,0});assert(releases==0); // A paused native clock cannot advance a lease.
    clockSeconds=4;field.Tick(audio,1,{0,0,100},{1,0,0});assert(!paused[1] && !paused[2] && paused[3] && releases==2 && field.FlashOpacity()==0);
    settings.grenades.flashStunAllUnits=true;settings.grenades.flashStunDistanceFalloff=false;
    // Full area stun includes allies, neutrals and the controlled thrower, with no damage or pre-paused lease theft.
    field.Reset();Scene();enumerated={1,2,3,4,5,6,7,8,9};for(int u=4;u<=9;++u)hp[u]=1000;hp[6]=0;
    assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));clockSeconds=.3f;field.Tick(audio,1,{0,0,100},{1,0,0});
    assert(paused[1] && paused[2] && paused[4] && paused[5] && paused[3] && pauses==4 && damages==0);
    assert(!paused[6] && !paused[7] && !paused[8] && !paused[9]);
    clockSeconds=2.29f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(paused[2]); // Full two seconds at every exposed distance.
    clockSeconds=2.31f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(!paused[1] && !paused[2] && !paused[4] && !paused[5] && paused[3] && releases==4);
    // Explicit legacy targeting excludes viewer/allies/neutrals; falling duration remains an independent opt-in.
    field.Reset();Scene();enumerated={1,2,4,5};hp[4]=hp[5]=1000;settings.grenades.flashStunAllUnits=false;settings.grenades.flashStunDistanceFalloff=true;
    assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));clockSeconds=.3f;field.Tick(audio,1,{0,0,100},{1,0,0});
    assert(!paused[1] && paused[2] && !paused[4] && !paused[5] && pauses==1);
    clockSeconds=2.29f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(!paused[2]);
    settings.grenades.flashStunAllUnits=true;settings.grenades.flashStunDistanceFalloff=false;
    // Overlapping flashes extend the adapter's existing lease, never double-pause or release it early.
    field.Reset();Scene();assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));
    clockSeconds=.1f;assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));
    clockSeconds=.21f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(pauses==2);
    clockSeconds=.31f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(pauses==2);
    clockSeconds=2.22f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(paused[2] && releases==0);
    clockSeconds=2.32f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(!paused[2] && releases==2 && paused[3]);
    // Geometry cover and explicit zero stun duration keep creatures active; removal of a thrower does not cancel area stun.
    field.Reset();Scene();cover={Bounds3{{12,-30,0},{14,30,200}}};assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));
    clockSeconds=.3f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(!paused[2]);
    field.Reset();Scene();settings.grenades.flashUnitStunSeconds=0;assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));
    clockSeconds=.3f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(pauses==0 && field.FlashOpacity()>0);settings.grenades.flashUnitStunSeconds=2;
    field.Reset();Scene();assert(field.Throw(1,WeaponSlots::Flashbang,{0,0,100},{1,0,0},{},settings));actorPresent=false;
    clockSeconds=.3f;field.Tick(audio,0,{0,0,100},{1,0,0});assert(paused[2] && pauses==1);
    field.Reset();Scene();settings.grenades.maxActive=1;
    settings.grenades.worldModelScale=1.25f;
    assert(field.Throw(1,WeaponSlots::Smoke,{0,0,100},{1,0,0},{},settings));assert(!field.Throw(1,WeaponSlots::HE,{},{1,0,0},{},settings));
    settings.grenades.worldModelScale=3;clockSeconds=.1f;field.Tick(audio,1,{0,0,100},{1,0,0});
    assert(field.Visuals().size()==1 && field.Visuals()[0].modelScale==1.25f && field.Visuals()[0].spin>0); // Launch snapshot owns world appearance.
    clockSeconds=2;field.Tick(audio,1,{0,0,100},{1,0,0});assert(field.Visuals().size()==1 && field.Visuals()[0].smoke && damages==0);
    // Read-only owner/sprite routing blocks both directions; neither target allegiance nor unit stats are consulted.
    alignas(uintptr_t) unsigned char source[0x40]={},victim[0x40]={},attack[0x40]={};
    *reinterpret_cast<uintptr_t*>(source+0x28)=1;*reinterpret_cast<uintptr_t*>(victim+0x28)=2;
    *reinterpret_cast<uintptr_t*>(attack+0x30)=uintptr_t(source);
    assert(field.BlocksSight(uintptr_t(attack),uintptr_t(victim)));
    *reinterpret_cast<uintptr_t*>(source+0x28)=2;*reinterpret_cast<uintptr_t*>(victim+0x28)=1;
    assert(field.BlocksSight(uintptr_t(attack),uintptr_t(victim)));
    assert(!field.BlocksSight(1,1)); // Unreadable objects fail safely rather than dereferencing stale memory.
    clockSeconds=30;field.Tick(audio,1,{0,0,100},{1,0,0});assert(field.Visuals().empty());
    assert(!field.BlocksSight(uintptr_t(attack),uintptr_t(victim)));
    field.Reset();Scene();cover={Bounds3{{12,-30,0},{14,30,200}}};
    assert(field.Throw(1,WeaponSlots::HE,{0,0,100},{1,0,0},{},settings));clockSeconds=.3f;field.Tick(audio,1,{0,0,100},{1,0,0});assert(hp[2]==1000 && hp[10]==1000);
    GameplaySettings stock;
    for(int slot:{WeaponSlots::HE,WeaponSlots::Flashbang,WeaponSlots::Smoke}) {
        field.Reset();Scene();
        assert(field.Throw(1,slot,{0,0,96},{1,0,0},{},stock));
        clockSeconds=.1f;field.Tick(audio,1,{0,0,96},{1,0,0});
        // Native routing must use the scaled CS speed and the distinct HE/flash/smoke gravity.
        assert(field.Visuals().size()==1 && field.Visuals()[0].position[0]>100);
        GrenadeProjectile expected;expected.position={23.63539f,0,100.16756f};expected.velocity={886.32697f,0,156.28336f};
        expected.settings.gravity=slot==WeaponSlots::HE ? 660.f : 600.f;
        expected.Advance(clockSeconds,{},[](float,float){return 0.f;});
        assert(std::abs(field.Visuals()[0].position[2]-expected.position[2])<.01f);
    }
    field.Reset();assert(field.Visuals().empty() && field.FlashOpacity()==0);
    // RPG shares native blast policy but owns pause-aware swept flight and one impact event.
    NativeRockets rockets;GameplaySettings rpg;rpg.rpg.available=true;rpg.friendlyFirePercent=0;rpg.rpg.maxActive=1;
    Scene();collisionFilters.clear();cover={Bounds3{{60,-100,0},{62,100,300}}};
    assert(rockets.Launch(1,{0,0,96},{1,0,0},{},rpg));assert(!rockets.Launch(1,{0,0,96},{1,0,0},{},rpg));
    // Later F8 damage/radius edits cannot alter the already launched 1800/600 blast snapshot.
    assert(rpg.damage[WeaponSlots::RPG]==1800 && rpg.rpg.radius==600);
    rpg.damage[WeaponSlots::RPG]=9999;rpg.rpg.radius=32;clockSeconds=.3f;assert(rockets.Tick(audio,1,false,{0,0,96},{1,0,0},3000));
    assert(hp[1]==1000 && hp[2]<1000 && hp[2]>-800 && hp[2]<100 && hp[10]<1000 && !rockets.Active(1));
    // Pathing-texture buildings may have zero native unit radius; rocket queries must still retain their model.
    assert(!collisionFilters.empty() && std::none_of(collisionFilters.begin(),collisionFilters.end(),[](bool value){return value;}));
    saved=damages;clockSeconds=2;rockets.Tick(audio,1,false,{0,0,96},{1,0,0},3000);assert(damages==saved);
    rockets.Reset();Scene();rpg.damage[WeaponSlots::RPG]=100;
    assert(rockets.Launch(1,{0,0,96},{1,0,0},{},rpg));clockSeconds=.2f;rockets.Tick(audio,1,false,{0,0,96},{1,0,0},3000);
    assert(rockets.Visuals().size()==1);auto pausedFlight=rockets.Visuals()[0].position;
    rockets.Tick(audio,1,false,{0,0,96},{1,0,0},3000);assert(rockets.Visuals()[0].position==pausedFlight);
    rockets.Reset();rpg.rpg.available=false;assert(!rockets.Launch(1,{0,0,96},{1,0,0},{},rpg));
    // Real HE/RPG adapters credit each lethal victim once, with no reward for their planter or repeated ticks.
    field.Reset();Scene();settings=GameplaySettings{};settings.grenades.fuse=.2f;settings.friendlyFirePercent=0;
    hp[2]=hp[3]=1;rewards=0;
    assert(field.Throw(1,WeaponSlots::HE,{0,0,100},{1,0,0},{},settings));clockSeconds=.3f;
    field.Tick(audio,1,{0,0,100},{1,0,0});assert(rewards==2);
    field.Tick(audio,1,{0,0,100},{1,0,0});assert(rewards==2);
    rockets.Reset();Scene();hp[2]=hp[3]=1;rewards=0;rpg=GameplaySettings{};rpg.rpg.available=true;rpg.friendlyFirePercent=0;
    rockets.SetKillSink([&](const KillReward& reward){assert(reward.owner==1);++rewards;});
    cover={Bounds3{{60,-100,0},{62,100,300}}};
    assert(rockets.Launch(1,{0,0,96},{1,0,0},{},rpg));clockSeconds=.3f;
    rockets.Tick(audio,1,false,{0,0,96},{1,0,0},3000);assert(rewards==2);
    clockSeconds=1;rockets.Tick(audio,1,false,{0,0,96},{1,0,0},3000);assert(rewards==2);
    std::puts("PASS native clock, single detonation, damage/friendly/cover/gate policy, F8 snapshots, flash leases and smoke expiry.");
}
