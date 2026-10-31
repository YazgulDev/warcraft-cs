#include "RuneCombatScene.hpp"
#include "BackgroundGameplay.hpp"
#include "../src/input/InputDispatcher.hpp"
#include "FramePresentationScene.hpp"
#include "../src/platform/NativeSpellSlots.hpp"
#include "../src/platform/WeaponTargetFilter.hpp"
#include "../src/platform/UnitCollisionProfile.hpp"
#include "../src/platform/SpriteTransform.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <array>

namespace {
void Mana(uintptr_t base,wc3::Handle unit,float value) {
    using SetState=void (__cdecl*)(wc3::Handle,int,float*);
    reinterpret_cast<SetState>(base+0x3C5EA0)(unit,2,&value);
}
template<class T> bool Read(uintptr_t at,T& value) {
    SIZE_T got=0;return at && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&value,sizeof(value),&got) && got==sizeof(value);
}
void Buttons(uintptr_t base) {
    // Opt-in read-only command-card telemetry is restricted to disposable test builds.
    using GetUI=uintptr_t (__fastcall*)(int,int);
    uintptr_t ui=reinterpret_cast<GetUI>(base+0x300710)(0,0),bar=0,items=0,rows=0,buttons=0;
    if(!Read(ui+0x3C8,bar) || !Read(bar+0x154,items) || !Read(items+8,rows) || !Read(rows,buttons))return;
    for(int i=0;i<12;++i) {
        uintptr_t data=0;std::array<unsigned char,0x6D8> bytes={};
        if(!Read(buttons+i*0x1C0+0x190,data) || !Read(data,bytes))continue;
        wc3::Log("fixture button index=%d ability=%08X order=%d flags=%08X object=%08X",i,
            *reinterpret_cast<int*>(bytes.data()+4),*reinterpret_cast<int*>(bytes.data()+8),
            *reinterpret_cast<int*>(bytes.data()+0x10),*reinterpret_cast<int*>(bytes.data()+0x6D4));
        for(size_t start=0;start<bytes.size();) {
            size_t end=start;while(end<bytes.size() && bytes[end]>=32 && bytes[end]<127)++end;
            if(end-start>=5)wc3::Log("fixture button string index=%d offset=%X text=%.*s",i,unsigned(start),int(end-start),bytes.data()+start);
            start=end+1;
        }
    }
}
}

void RuneCombatScene::Tick(ShooterController& controller,uintptr_t base,const char* root) {
    static std::vector<wc3::Handle> items;
    static std::vector<wc3::Handle> party;
    static std::vector<wc3::Handle> nuclearScenery;
    static wc3::Handle enemy=0,blocker=0,motionActor=0;static DWORD check=0,observeUntil=0;
    static DWORD motionUntil=0;
    // A disposable per-render audit distinguishes physical placement jumps from cached camera latency.
    if(BackgroundGameplay::Enabled() && controller.Visible() && GetTickCount()<motionUntil) {
        LARGE_INTEGER clock;QueryPerformanceCounter(&clock);auto actor=controller.TestUnit();
        wc3::Log("fixture motion clock=%lld x=%.5f y=%.5f feet=%.5f speed=%.5f eyeX=%.5f eyeY=%.5f eyeZ=%.5f wantedZ=%.5f",
            clock.QuadPart,wc3::Real(wc3::GetUnitX(actor)),wc3::Real(wc3::GetUnitY(actor)),controller.ActorFeet(),controller.DisplaySpeed(),
            wc3::Real(wc3::GetCameraEyePositionX()),wc3::Real(wc3::GetCameraEyePositionY()),wc3::Real(wc3::GetCameraEyePositionZ()),controller.ViewEyeZ());
    }
    if ((!controller.Visible() && !BackgroundGameplay::Enabled()) || GetTickCount()-check<250) return;
    check=GetTickCount();auto unit=controller.TestUnit();
    std::string path=std::string(root)+"\\test-gameplay.request";
    if (GetFileAttributesA(path.c_str())!=INVALID_FILE_ATTRIBUTES) {
        std::ifstream file(path);std::string line;std::getline(file,line);file.close();DeleteFileA(path.c_str());
        std::istringstream input(line);std::string command;input>>command;
        // The background oracle uses the same controller mailboxes without sending desktop input.
        if(BackgroundGameplay::Enabled() && command=="toggle")controller.RequestToggle();
        else if(BackgroundGameplay::Enabled() && command=="lowstep") {
            // A real native low prop exercises abrupt support changes without a synthetic physics floor.
            float x=0,y=0,angle=0,scale=.2f;input>>x>>y>>scale;scale=std::clamp(scale,.05f,1.f);
            wc3::CreateDestructable(0x4C546261,&x,&y,&angle,&scale,0);
        }
        else if(BackgroundGameplay::Enabled() && command=="motionaudit") {
            DWORD ms=0;input>>ms;motionUntil=GetTickCount()+std::min(ms,DWORD(20000));
            wc3::Log("fixture motion start smoothing=%d seconds=%.3f",controller.Settings().cameraHeight.enabled,controller.Settings().cameraHeight.seconds);
        }
        else if(BackgroundGameplay::Enabled() && command=="swapbackend") {
            // Reproduce frame completion without the old swap hook after a restored campaign.
            int value=0;input>>value;FramePresentationScene::Alternate(value!=0);
        }
        else if(BackgroundGameplay::Enabled() && (command=="savehud" || command=="loadhud")) {
            // Save/load the disposable campaign without F10, which would disable FPS first.
            const char* name="HudCampaignTest.w3z";
            uintptr_t node[8]={},wrapper[3]={};node[7]=reinterpret_cast<uintptr_t>(name);wrapper[2]=reinterpret_cast<uintptr_t>(node);
            if(command=="savehud") {
                using Save=void (__cdecl*)(wc3::Handle);
                reinterpret_cast<Save>(base+0x3BBEE0)(reinterpret_cast<wc3::Handle>(wrapper));
            } else {
                using Load=void (__cdecl*)(wc3::Handle,BOOL);
                reinterpret_cast<Load>(base+0x3BBE60)(reinterpret_cast<wc3::Handle>(wrapper),FALSE);
            }
            wc3::Log("fixture campaign %s requested fps=%d",command.c_str(),controller.Visible());
        }
        else if(BackgroundGameplay::Enabled() && command=="view")controller.RequestViewToggle();
        else if(BackgroundGameplay::Enabled() && command=="third")controller.RequestThirdPersonToggle();
        else if(BackgroundGameplay::Enabled() && command=="unitview")controller.RequestUnitViewToggle();
        // Observe production profile gates after queued inputs settle, without touching desktop input.
        else if(BackgroundGameplay::Enabled() && command=="profile")
            wc3::Log("fixture profile only=%d active=%d third=%d unitview=%d buying=%d",
                controller.Settings().warcraftOnly,controller.Visible(),controller.ThirdPerson(),controller.UnitFirstPerson(),controller.Buying());
        else if(BackgroundGameplay::Enabled() && command=="hoverstats") {
            // Inspect the live native switches and actual camera, not only the configured snapshot.
            using UI=uintptr_t (__fastcall*)(int,int);auto ui=reinterpret_cast<UI>(base+0x300710)(0,0);
            if(ui)wc3::Log("fixture hover input=%d ui=%d hide=%d view=%d far=%.1f configured=%.1f",
                *reinterpret_cast<int*>(ui+0x298),*reinterpret_cast<int*>(ui+0x29C),controller.Settings().unitViewHideHoverCircles,
                controller.UnitFirstPerson(),wc3::Real(wc3::GetCameraField(1)),controller.Settings().render.drawDistance);
        }
        else if(BackgroundGameplay::Enabled() && command=="hoverrequest") {
            // Route a map's EnablePreSelect request through its real public native entry while F9 is active.
            int enabled=1,visual=1;input>>enabled>>visual;
            reinterpret_cast<void (__cdecl*)(BOOL,BOOL)>(base+0x3B3B50)(enabled,visual);
        }
        // Exercise Q's game-thread path without a desktop key or a fixture-only targeting policy.
        else if(BackgroundGameplay::Enabled() && command=="allytoggle")controller.RequestAllyTargetToggle();
        // Exercise production wheel routing without injecting input into the user's desktop.
        else if(BackgroundGameplay::Enabled() && command=="wheel") {int delta=0;input>>delta;controller.RequestMouseWheel(delta);}
        else if(BackgroundGameplay::Enabled() && command=="heroattack")controller.TestHeroAttack();
        else if(BackgroundGameplay::Enabled() && command=="holdattack") {DWORD duration=0;input>>duration;controller.TestHoldAttack(duration);}
        else if(BackgroundGameplay::Enabled() && command=="targetstats")controller.TestLogTarget();
        else if(BackgroundGameplay::Enabled() && (command=="aimhead" || command=="headstrike") && enemy) {
            // Verification uses the same named-head ray and shooter damage as real inputs; no forced kill or body flag.
            int side=0;input>>side;
            if(controller.TestAimHead(enemy,side) && command=="headstrike") {
                controller.TestStrike(WeaponSlots::AWP);
                // Keep the first native death pose visible; ordinary modules never freeze a corpse.
                wc3::AnimationRate(enemy,0);
                wc3::Log("fixture head corpse sequence=%d active=%d",wc3::AnimationSequence(enemy),wc3::ActiveAnimationSequence(enemy));
            }
        }
        else if(BackgroundGameplay::Enabled() && command=="enemyrate" && enemy) {
            // Freeze only the disposable corpse pose for visual inspection before native decay.
            float rate=1;input>>rate;wc3::AnimationRate(enemy,rate);
        }
        else if(BackgroundGameplay::Enabled() && command=="enemypause" && enemy) {
            // Let a newly created native model initialize its pose before freezing an oracle target.
            int value=1;input>>value;wc3::PauseUnit(enemy,value!=0);
        }
        else if(BackgroundGameplay::Enabled() && command=="enemyhealth" && enemy) {
            float health=100;input>>health;using SetState=void (__cdecl*)(wc3::Handle,int,float*);
            reinterpret_cast<SetState>(base+0x3C5EA0)(enemy,0,&health);
        }
        else if(BackgroundGameplay::Enabled() && command=="waterenemy") {
            // Force only disposable targets onto authored shallow/deep-water cells, without pathing relocation.
            float x=256,y=0,facing=180,scale=1;std::string raw="hfoo";input>>x>>y>>raw>>scale;
            if(enemy && wc3::GetUnitTypeId(enemy))wc3::RemoveUnit(enemy);
            enemy=wc3::CreateUnit(wc3::Player(12),HeroSpellSlots::Rawcode(raw),&x,&y,&facing);
            wc3::SetUnitX(enemy,&x);wc3::SetUnitY(enemy,&y);wc3::PauseUnit(enemy,TRUE);
            if(std::isfinite(scale) && scale>=.1f && scale<=10)wc3::SetUnitScale(enemy,&scale,&scale,&scale);
        }
        else if(BackgroundGameplay::Enabled() && (command=="waterprobe" || command=="aimwater") && enemy) {
            // Read native surface modes and the rendered transform before trusting a water hit-test hypothesis.
            auto object=wc3::UnitObject(enemy);uintptr_t sprite=0;Read(object+0x28,sprite);
            ModelTransform transform;SpriteTransform::Read(sprite,transform);
            float x=wc3::Real(wc3::GetUnitX(enemy)),y=wc3::Real(wc3::GetUnitY(enemy)),position[]={x,y,0};
            using Root=uintptr_t (__cdecl*)();using Height=float (__thiscall*)(uintptr_t,const float*,int);
            auto terrain=reinterpret_cast<Root>(base+0x1F5A0)();auto height=reinterpret_cast<Height>(base+0x763F00);
            float cachedZ=0;Read(sprite+0xC8,cachedZ);
            wc3::Log("fixture water x=%.1f y=%.1f location=%.1f terrain=%.1f mode2=%.1f raw=%.1f model=%.1f cached=%.1f fly=%.1f",
                x,y,wc3::Ground(x,y),wc3::TerrainGround(x,y),height(terrain,position,-2),height(terrain,position,-3),
                transform.position[2],cachedZ,wc3::Real(wc3::GetUnitFlyHeight(enemy)));
            if(command=="aimwater") {
                float offset=60;input>>offset;
                controller.TestAimPoint(x,y,transform.position[2]+offset);
            }
            wc3::Log("fixture water solid=%.1f legacy=%.1f",wc3::AttackSurface(x,y,false),wc3::AttackSurface(x,y,true));
        }
        else if(BackgroundGameplay::Enabled() && command=="enemyattack") {
            // The normal script-order hook must reject F9 targets and accept them after F8/exit.
            wc3::Log("fixture enemy attack accepted=%d",enemy && unit ? wc3::IssueTargetOrderById(enemy,851983,unit) : -1);
        }
        else if(BackgroundGameplay::Enabled() && command=="enemyattackother") {
            // Protection is actor-specific: an enemy must still be able to fight another owned unit.
            float x=wc3::Real(wc3::GetUnitX(unit))+150,y=wc3::Real(wc3::GetUnitY(unit))+120,angle=0;
            auto other=wc3::CreateUnit(wc3::GetLocalPlayer(),0x68666F6F,&x,&y,&angle);party.push_back(other);
            wc3::Log("fixture other attack accepted=%d",wc3::IssueTargetOrderById(enemy,851983,other));
        }
        else if(BackgroundGameplay::Enabled() && command=="otherstats" && !party.empty())
            wc3::Log("fixture other hp=%.1f",wc3::Real(wc3::GetUnitState(party.back(),0)));
        else if(BackgroundGameplay::Enabled() && command=="pause") {int value=0;input>>value;wc3::PauseUnit(unit,value!=0);}
        else if(BackgroundGameplay::Enabled() && command=="camerawall") {
            // A tall, solid native building behind the actor probes boom collision without altering terrain.
            float x=wc3::Real(wc3::GetUnitX(unit))-220,y=wc3::Real(wc3::GetUnitY(unit)),facing=0;
            party.push_back(wc3::CreateUnit(wc3::GetLocalPlayer(),0x68746F77,&x,&y,&facing));
        }
        else if(BackgroundGameplay::Enabled() && command=="walk") {std::string key,gait;DWORD ms=0;input>>key>>ms>>gait;controller.TestWalk(ShortcutBindings::Parse(key),ms,gait=="slow");}
        else if(BackgroundGameplay::Enabled() && command=="motionstats") {
            controller.TestAnimationState();
            // Read the native clip clock to distinguish progressing playback from repeated frame-zero repairs.
            auto actor=wc3::UnitObject(unit);uintptr_t sprite=0,model=0,animation=0,frames=0,metadata=0,clips=0;
            int sequence=wc3::ActiveAnimationSequence(unit),frame=0,begin=0;
            if(sequence>=0 && sequence<256 && Read(actor+0x28,sprite) && Read(sprite+0x20,model) &&
                Read(model+0x98,animation) && Read(animation+8,frames) && Read(animation+0x44,metadata) &&
                Read(metadata+0x18,clips) && Read(frames+sequence*16,frame) && Read(clips+sequence*0x8C,begin))
                wc3::Log("fixture clip sequence=%d elapsedMs=%d",sequence,frame-begin);
            // The sprite's requested index can differ from the model's active blend/clip during crowd contact.
            unsigned char active=255;short queued=-1;
            if(Read(sprite+0x2C,queued) && Read(animation+0x58,active))
                wc3::Log("fixture clip active=%u queued=%d blending=%d",unsigned(active),queued,wc3::AnimationBlending(unit));
            wc3::Log("fixture camera distance=%.1f third=%d weapon=%d",controller.ThirdPersonDistance(),controller.ThirdPerson(),controller.WeaponIndex());
            // This oracle observes restored simulation height/rate, outside the transient world render scope.
            wc3::Log("fixture motion fly=%.2f rate=%.2f feet=%.2f",wc3::Real(wc3::GetUnitFlyHeight(unit)),wc3::AnimationRate(unit),controller.ActorFeet());
            auto body=UnitCollisionProfile::Read(unit,base);
            wc3::Log("fixture body known=%d colliding=%d radius=%.1f",body.known,body.colliding,body.radius);
            wc3::Log("fixture animation sequence=%d",wc3::AnimationSequence(unit));
        }
        else if(BackgroundGameplay::Enabled() && command=="forcepose") {
            // Reproduce a native AI overwrite while the controller's cached locomotion state remains unchanged.
            int index=0;input>>index;wc3::AnimateIndex(unit,index);
            wc3::Log("fixture forced sequence=%d observed=%d",index,wc3::AnimationSequence(unit));
        }
        else if(BackgroundGameplay::Enabled() && command=="position") {
            // Exact placement is confined to a disposable oracle; normal controllers never teleport actors.
            if(!controller.Visible())unit=wc3::PickOwnedUnit();
            float x=0,y=0;input>>x>>y;wc3::SetUnitX(unit,&x);wc3::SetUnitY(unit,&y);
            wc3::Log("fixture position x=%.1f y=%.1f terrain=%.1f ground=%.1f",x,y,wc3::TerrainGround(x,y),wc3::Ground(x,y));
        }
        else if(BackgroundGameplay::Enabled() && command=="obstacles") {
            // Identify map helpers that visually overlap a ramp but have no actual Warcraft collision.
            float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit)),range=600;
            auto group=wc3::CreateGroup();wc3::GroupEnumUnitsInRange(group,&x,&y,&range,0);
            while(auto other=wc3::FirstOfGroup(group)) {
                wc3::GroupRemoveUnit(group,other);
                wc3::Log("fixture obstacle unit=%08X type=%08X x=%.1f y=%.1f locust=%d transparent=%d model=%s",other,wc3::GetUnitTypeId(other),
                    wc3::Real(wc3::GetUnitX(other)),wc3::Real(wc3::GetUnitY(other)),wc3::GetUnitAbilityLevel(other,0x416C6F63),
                    WeaponTargetFilter::PassThrough(other,base),wc3::UnitModelPath(wc3::GetUnitTypeId(other)).c_str());
            }
            wc3::DestroyGroup(group);
        }
        else if(BackgroundGameplay::Enabled() && command=="spell") {int slot=0;input>>slot;controller.RequestSpell(slot-1);}
        else if(BackgroundGameplay::Enabled() && command=="bars")controller.RequestHealthBarToggle();
        else if(BackgroundGameplay::Enabled() && command=="minimap")controller.RequestMinimapToggle();
        else if(BackgroundGameplay::Enabled() && command=="inventory") {
            // Drive production inventory gestures in the disposable oracle without desktop input.
            std::string action;input>>action;
            if(action=="panel")controller.RequestInventory({InventoryControls::Kind::Panel});
            else if(action=="close")controller.RequestInventoryClose();
            else if(action=="use") {int slot=1;input>>slot;InventoryControls::Event event{InventoryControls::Kind::Use};event.slot=slot-1;event.self=true;controller.RequestInventory(event);}
            else if(action=="down" || action=="up" || action=="move") {
                InventoryControls::Event event{action=="down" ? InventoryControls::Kind::Down : action=="up" ? InventoryControls::Kind::Up : InventoryControls::Kind::Move};
                input>>event.x>>event.y>>event.width>>event.height;controller.RequestInventory(event);
            }
        }
        else if(BackgroundGameplay::Enabled() && command=="buttons")Buttons(base);
        // New tuning follows production mailboxes; only disposable fixture builds expose these commands.
        else if(BackgroundGameplay::Enabled() && command=="settings")controller.RequestSettingsReload();
        else if(BackgroundGameplay::Enabled() && command=="settingskey") {
            // Call the production dispatcher directly with this process's own window, without desktop input.
            int key=0;input>>key;InputDispatcher dispatcher(controller);controller.TestInputRoute(true);
            dispatcher.Handle(GetActiveWindow(),WM_KEYDOWN,WPARAM(key),0,true);
            dispatcher.Handle(GetActiveWindow(),WM_KEYUP,WPARAM(key),1L<<31,true);controller.TestInputRoute(false);
        }
        else if(BackgroundGameplay::Enabled() && command=="textchar") {
            unsigned codepoint=0;input>>codepoint;InputDispatcher dispatcher(controller);controller.TestInputRoute(true);
            dispatcher.Handle(GetActiveWindow(),WM_UNICHAR,codepoint,0,true);controller.TestInputRoute(false);
        }
        else if(BackgroundGameplay::Enabled() && command=="textline") {
            // Simulate text characters through the production dispatcher; no desktop keystrokes are injected.
            std::string typedLine;std::getline(input,typedLine);if(!typedLine.empty() && typedLine[0]==' ')typedLine.erase(0,1);
            InputDispatcher dispatcher(controller);controller.TestInputRoute(true);
            for(unsigned char c:typedLine)dispatcher.Handle(GetActiveWindow(),WM_CHAR,c,0,true);controller.TestInputRoute(false);
        }
        else if(BackgroundGameplay::Enabled() && command=="nuclearinspect") {
            wc3::Log("fixture nuclear textOpen=%d selected=%d active=%d remaining=%.2f age=%.2f serial=%u ammo=%d gold=%d speed=%.2f",controller.TextInputOpen(),controller.NuclearSelected(),controller.NuclearBomb().Active(),controller.NuclearBomb().Remaining(),controller.NuclearBomb().Age(),controller.ShotSerial(),controller.Ammo(),wc3::GetPlayerState(wc3::GetLocalPlayer(),1),controller.DisplaySpeed());
        }
        else if(BackgroundGameplay::Enabled() && command=="nuclearimpactscene") {
            party.clear();nuclearScenery.clear();float x=-1000,y=-1000,angle=0,scale=1;
            const int owners[]={12,12,15,0,1,15,12,15,12};
            const int types[]={0x6E6F6772,0x75676172,0x6E746176,0x68666F6F,0x68746F77,0x6E736865,0x75637279,0x6E746176,0x6E6F6772};
            const float dx[]={200,300,-50,300,600,450,3500,3500,600},dy[]={250,450,100,600,600,-350,1800,2200,-200};
            using Add=BOOL (__cdecl*)(wc3::Handle,int);
            for(int i=0;i<9;++i){float px=x+dx[i],py=y+dy[i];auto target=wc3::CreateUnit(wc3::Player(owners[i]),types[i],&px,&py,&angle);party.push_back(target);if(target)wc3::PauseUnit(target,TRUE);if(i==2 || i==8)reinterpret_cast<Add>(base+0x3C82A0)(target,0x4176756C);}
            for(int i=0;i<3;++i){float px=x+(i==2 ? 4000.f : 400+i*200.f),py=y-200;auto prop=wc3::CreateDestructable(i==1 ? 0x4C546261 : 0x4C546C74,&px,&py,&angle,&scale,0);nuclearScenery.push_back(prop);
                if(i==1)reinterpret_cast<void (__cdecl*)(wc3::Handle,BOOL)>(base+0x3C4770)(prop,TRUE);}
            controller.TestAimPoint(x,y,8);wc3::Log("fixture nuclear impact scene units=%u props=%u",unsigned(party.size()),unsigned(nuclearScenery.size()));
        }
        else if(BackgroundGameplay::Enabled() && command=="nuclearaim") {float x=0,y=0,z=0;input>>x>>y>>z;controller.TestAimPoint(x,y,z);}
        else if(BackgroundGameplay::Enabled() && command=="nuclearclick") {
            InputDispatcher dispatcher(controller);controller.TestInputRoute(true);
            dispatcher.Handle(GetActiveWindow(),WM_LBUTTONDOWN,MK_LBUTTON,0,true);dispatcher.Handle(GetActiveWindow(),WM_LBUTTONUP,0,0,true);controller.TestInputRoute(false);
        }
        else if(BackgroundGameplay::Enabled() && command=="nuclearscene") {
            // A disposable mixed team fixture verifies map-wide air/ground kills and preserved allies/buildings.
            party.clear();float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit)),angle=0;
            const int owners[]={12,12,12,0,12};const int types[]={0x6E6F6772,0x75676172,0x756E706C,0x68666F6F,0x68746F77};
            for(int i=0;i<5;++i){float px=x+float(i+1)*260,py=y+150;auto target=wc3::CreateUnit(wc3::Player(owners[i]),types[i],&px,&py,&angle);party.push_back(target);if(target)wc3::PauseUnit(target,TRUE);}
            // An ordinary native invulnerable creature must survive as well.
            float px=x+260,py=y+600;auto protectedUnit=wc3::CreateUnit(wc3::Player(12),0x6E6F6772,&px,&py,&angle);party.push_back(protectedUnit);
            float distantX=x+3500,distantY=y+1800;auto distant=wc3::CreateUnit(wc3::Player(12),0x75637279,&distantX,&distantY,&angle);party.push_back(distant);wc3::PauseUnit(distant,TRUE);
            using Add=BOOL (__cdecl*)(wc3::Handle,int);reinterpret_cast<Add>(base+0x3C82A0)(protectedUnit,0x4176756C);
            wc3::PauseUnit(protectedUnit,TRUE);wc3::Log("fixture nuclear scene created=%u",unsigned(party.size()));
        }
        else if(BackgroundGameplay::Enabled() && command=="nuclearsceneinspect") {
            for(size_t i=0;i<party.size();++i)wc3::Log("fixture nuclear target index=%u type=%08X hp=%.2f x=%.1f y=%.1f",unsigned(i),wc3::GetUnitTypeId(party[i]),wc3::GetUnitTypeId(party[i]) ? wc3::Real(wc3::GetUnitState(party[i],0)) : 0,wc3::Real(wc3::GetUnitX(party[i])),wc3::Real(wc3::GetUnitY(party[i])));
            for(size_t i=0;i<nuclearScenery.size();++i)wc3::Log("fixture nuclear prop index=%u type=%08X hp=%.2f",unsigned(i),wc3::GetDestructableTypeId(nuclearScenery[i]),wc3::Real(wc3::GetDestructableLife(nuclearScenery[i])));
        }
        else if(BackgroundGameplay::Enabled() && command=="settingssky")controller.TestLogSky();
        else if(BackgroundGameplay::Enabled() && command=="settingsmapsky")controller.TestMapSky("Environment\\Sky\\LordaeronWinterSky\\LordaeronWinterSky.mdl");
        else if(BackgroundGameplay::Enabled() && command=="settingsrow") {int row=0;float steps=0;input>>row>>steps;controller.RequestSettingsMenu({SettingsMenu::Kind::Adjust,row,steps});}
        else if(BackgroundGameplay::Enabled() && command=="settingsclick") {
            int x=0,y=0,w=0,h=0;input>>x>>y>>w>>h;controller.RequestSettingsClick(x,y,w,h);
        }
        else if(BackgroundGameplay::Enabled() && command=="settingsinspect") {
            // Values expose the actual applied snapshot, not merely the pending draft displayed by the menu.
            auto& s=controller.Settings();wc3::Log("fixture settings open=%d dirty=%d sensitivity=%.3f scope=%.3f volume=%.1f draw=%.0f classic=%d recovery=%d sky=%d csSky=%d preset=%s visible=%d ammo=%d serial=%u",
                controller.SettingsPanel().Open(),controller.SettingsPanel().Dirty(),s.mouse.sensitivity,s.mouse.scopeSensitivity,s.csVolumePercent,s.render.drawDistance,s.movement.classicCollision,s.movement.platformRecovery,s.skyVisible,s.csSky,controller.SkyName().c_str(),controller.Visible(),controller.Ammo(),controller.ShotSerial());
        }
        else if(BackgroundGameplay::Enabled() && command=="equip") {int slot=-1;input>>slot;controller.TestEquip(slot);}
        else if(BackgroundGameplay::Enabled() && command=="reload")controller.TestReload();
        else if(BackgroundGameplay::Enabled() && command=="secondary")controller.TestSecondary();
        else if(BackgroundGameplay::Enabled() && command=="arsenal")controller.TestArsenalState();
        else if(BackgroundGameplay::Enabled() && command=="smoke") {float x=0,y=0,z=0;input>>x>>y>>z;controller.TestSmoke(x,y,z);}
        else if(BackgroundGameplay::Enabled() && command=="refill")controller.RequestRefill();
        else if(BackgroundGameplay::Enabled() && command=="grant")controller.RequestAllWeapons();
        else if(BackgroundGameplay::Enabled() && command=="pickup")controller.RequestItemPickup();
        else if(BackgroundGameplay::Enabled() && command=="buy")controller.RequestBuyToggle();
        else if(BackgroundGameplay::Enabled() && command=="buykey") {int key=0;input>>key;controller.RequestBuyKey(key);}
        else if(BackgroundGameplay::Enabled() && command=="haste") {
            float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit));
            auto item=wc3::CreateItem(0x6763656C,&x,&y);items.push_back(item);
            wc3::Log("fixture haste added=%d",wc3::UnitAddItem(unit,item));
        }
        else if(BackgroundGameplay::Enabled() && command=="follow")controller.RequestSquad(true);
        else if(BackgroundGameplay::Enabled() && command=="fight")controller.RequestSquad(false);
        else if(BackgroundGameplay::Enabled() && command=="release")controller.RequestSquadRelease();
        else if(BackgroundGameplay::Enabled() && command=="strike") {
            // Aim/target creation uses the existing private request; this exercises production contact damage.
            int slot=2;input>>slot;controller.TestStrike(slot);
            // Read confirmation immediately, before short body particles can expire between mailbox requests.
            wc3::Log("fixture impact visuals blood=%u captions=%u",unsigned(controller.Visuals().BloodDrops().size()),unsigned(controller.Visuals().Criticals().size()));
        }
        else if(BackgroundGameplay::Enabled() && command=="blocker") {
            // A separate disposable effect/physical body tests the production nearest-hit selection.
            if(blocker && wc3::GetUnitTypeId(blocker))wc3::RemoveUnit(blocker);blocker=0;
            std::string raw;float distance=150,fly=-1;int invulnerable=1,pathing=1,owner=12,reveal=0,exact=0;input>>raw>>distance>>fly>>invulnerable>>pathing>>owner>>reveal>>exact;
            if(raw.size()==4) {
                int id=(raw[0]<<24)|(raw[1]<<16)|(raw[2]<<8)|raw[3];float facing=180;
                float x=wc3::Real(wc3::GetUnitX(unit))+distance,y=wc3::Real(wc3::GetUnitY(unit));
                blocker=wc3::CreateUnit(wc3::Player(owner>=0 && owner<=15 ? owner : 12),id,&x,&y,&facing);wc3::PauseUnit(blocker,TRUE);
                // A native building may snap sideways on creation. Exact fixture placement tests real in-line occlusion.
                if(exact){wc3::SetUnitX(blocker,&x);wc3::SetUnitY(blocker,&y);}
                using Switch=void (__cdecl*)(wc3::Handle,BOOL);
                reinterpret_cast<Switch>(base+0x3C72F0)(blocker,invulnerable!=0);
                reinterpret_cast<Switch>(base+0x3C5FA0)(blocker,pathing!=0);
                if(fly>=0) {
                    using Height=void (__cdecl*)(wc3::Handle,float*,float*);float rate=0;
                    reinterpret_cast<Height>(base+0x3C5C60)(blocker,&fly,&rate);
                }
                // Map hide/show lifecycles can make a Locust effect enumerable without removing its ability.
                // Reproduce that visible state in this fixture rather than altering the production group query.
                if(reveal) {reinterpret_cast<Switch>(base+0x3CE730)(blocker,FALSE);reinterpret_cast<Switch>(base+0x3CE730)(blocker,TRUE);}
                wc3::Log("fixture blocker unit=%08X type=%08X locust=%d transparent=%d fly=%.1f hp=%.1f visible=%d owner=%d x=%.1f y=%.1f",blocker,id,
                    wc3::GetUnitAbilityLevel(blocker,0x416C6F63),WeaponTargetFilter::PassThrough(blocker,base),
                    wc3::Real(wc3::GetUnitFlyHeight(blocker)),wc3::Real(wc3::GetUnitState(blocker,0)),wc3::IsUnitVisible(blocker,wc3::GetLocalPlayer()),owner,wc3::Real(wc3::GetUnitX(blocker)),wc3::Real(wc3::GetUnitY(blocker)));
            }
        }
        else if(BackgroundGameplay::Enabled() && command=="blockerstats") {
            // Read back native health to prove that passthrough never damages the protected foreground ally.
            if(blocker && wc3::GetUnitTypeId(blocker))wc3::Log("fixture blocker stats hp=%.1f allied=%d",
                wc3::Real(wc3::GetUnitState(blocker,0)),wc3::IsUnitAlly(blocker,wc3::GetOwningPlayer(unit)));
        }
        else if(BackgroundGameplay::Enabled() && command=="passive") {
            // Stock abilities are supplied by the disposable map, never bundled with the mod.
            std::string raw;input>>raw;
            if(raw.size()==4) {
                int id=(raw[0]<<24)|(raw[1]<<16)|(raw[2]<<8)|raw[3];
                using Add=BOOL (__cdecl*)(wc3::Handle,int);
                BOOL added=reinterpret_cast<Add>(base+0x3C82A0)(unit,id);
                wc3::Log("fixture passive ability=%08X added=%d level=%d",id,added,wc3::GetUnitAbilityLevel(unit,id));
            }
        }
        else if(BackgroundGameplay::Enabled() && command=="item") {
            std::string raw;input>>raw;
            if(raw.size()==4) {
                float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit));
                int id=(raw[0]<<24)|(raw[1]<<16)|(raw[2]<<8)|raw[3];
                auto item=wc3::CreateItem(id,&x,&y);items.push_back(item);
                wc3::Log("fixture passive item=%08X added=%d",id,wc3::UnitAddItem(unit,item));
            }
        }
        else if(BackgroundGameplay::Enabled() && command=="health") {
            float health=0;input>>health;
            using SetState=void (__cdecl*)(wc3::Handle,int,float*);
            reinterpret_cast<SetState>(base+0x3C5EA0)(unit,0,&health);
        }
        else if(BackgroundGameplay::Enabled() && command=="dota") {
            // Spawn/learn the current map's own Anti-Mage definitions, including its skill-learning triggers.
            float x=0,y=0,facing=0;auto hero=wc3::CreateUnit(wc3::GetLocalPlayer(),0x4564656D,&x,&y,&facing);
            using Level=void (__cdecl*)(wc3::Handle,int,BOOL);using Learn=void (__cdecl*)(wc3::Handle,int);
            reinterpret_cast<Level>(base+0x3C6D70)(hero,7,FALSE);
            NativeSpellSlots slots;slots.Configure(base);SpellSettings defaults;
            for(int i=0;i<4;++i) {int id=slots.Ability(hero,i,defaults);if(id)reinterpret_cast<Learn>(base+0x3C7050)(hero,id);}
            Mana(base,hero,1000);
            using Clear=void (__cdecl*)();using Select=void (__cdecl*)(wc3::Handle,BOOL);
            reinterpret_cast<Clear>(base+0x3BBAA0)();reinterpret_cast<Select>(base+0x3C7910)(hero,TRUE);
            controller.RequestToggle();wc3::Log("fixture dota actor=%08X",hero);
        }
        else if(BackgroundGameplay::Enabled() && command=="nativewalk") {
            // Exercise an already-started native walking motor, rather than only rejected attack orders.
            float x=wc3::Real(wc3::GetUnitX(unit))+500,y=wc3::Real(wc3::GetUnitY(unit));
            wc3::IssuePointOrderById(unit,851986,&x,&y);
            wc3::Log("fixture native walk speed=%.1f",wc3::Real(wc3::GetUnitMoveSpeed(unit)));
        }
        else if(BackgroundGameplay::Enabled() && command=="front") {
            float x=wc3::Real(wc3::GetUnitX(unit))+130,y=wc3::Real(wc3::GetUnitY(unit))+20,facing=180;
            party.push_back(wc3::CreateUnit(wc3::GetLocalPlayer(),0x68666F6F,&x,&y,&facing));
            y-=40;party.push_back(wc3::CreateUnit(wc3::GetLocalPlayer(),0x68666F6F,&x,&y,&facing));
        }
        else if(BackgroundGameplay::Enabled() && command=="crowd") {
            // Deliberate close overlaps reproduce moving combat neighbours without using a user's campaign.
            float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit)),facing=180;
            for(int i=0;i<6;++i) {
                float cx=x+35+(i%2)*15,cy=y+(i/2-1)*28;
                auto neighbour=wc3::CreateUnit(i<4 ? wc3::Player(12) : wc3::GetLocalPlayer(),0x68666F6F,&cx,&cy,&facing);
                wc3::SetUnitX(neighbour,&cx);wc3::SetUnitY(neighbour,&cy);party.push_back(neighbour);
                if(i<4)wc3::IssueTargetOrderById(neighbour,851983,unit);
            }
        }
        else if(BackgroundGameplay::Enabled() && command=="spellparty") {
            // Disposable allied casters exercise real mana/cooldown/channel ownership; normal maps gain no abilities.
            float x=wc3::Real(wc3::GetUnitX(unit))-220,y=wc3::Real(wc3::GetUnitY(unit))+120,facing=0;
            auto mage=wc3::CreateUnit(wc3::GetLocalPlayer(),0x486D6B67,&x,&y,&facing);party.push_back(mage);
            using Level=void (__cdecl*)(wc3::Handle,int,BOOL);using Learn=void (__cdecl*)(wc3::Handle,int);
            reinterpret_cast<Level>(base+0x3C6D70)(mage,10,FALSE);reinterpret_cast<Learn>(base+0x3C7050)(mage,0x41487462);
            Mana(base,mage,1000);
            using Add=BOOL (__cdecl*)(wc3::Handle,int);
            reinterpret_cast<Add>(base+0x3C82A0)(mage,0x41487462);
            x-=100;auto priest=wc3::CreateUnit(wc3::GetLocalPlayer(),0x686D7072,&x,&y,&facing);party.push_back(priest);Mana(base,priest,300);
            // Turn off the fixture priest's stock autocast so healing proves the new squad decision path.
            if(int off=wc3::OrderId("healoff"))wc3::IssueImmediateOrderById(priest,off);
            wc3::Log("fixture spellparty mage=%08X priest=%08X boltLevel=%d healLevel=%d",mage,priest,wc3::GetUnitAbilityLevel(mage,0x41487462),wc3::GetUnitAbilityLevel(priest,0x41686561));
        }
        else if(BackgroundGameplay::Enabled() && command=="mana") {
            float mana=0;input>>mana;Mana(base,unit,mana);wc3::Log("fixture mana=%.1f",wc3::Real(wc3::GetUnitState(unit,2)));
        }
        else if (command=="hero" || command=="creep" || command=="arthas" || (BackgroundGameplay::Enabled() && command=="actor")) {
            // A native hero has an inventory, unlike an unupgraded starting worker.
            // Move the private actor away from the starting workers so squad membership has an exact oracle.
            float x=wc3::Real(wc3::GetUnitX(unit))-1000,y=wc3::Real(wc3::GetUnitY(unit)),facing=0;
            std::string raw;if(command=="actor")input>>raw;
            int type=command=="actor" ? HeroSpellSlots::Rawcode(raw) : command=="arthas" ? 0x48617274 : command=="hero" ? 0x4870616C : 0x68666F6F;
            // Only marked disposable maps can spawn arbitrary owned actors for locomotion verification.
            if(command=="actor" && motionActor && wc3::GetUnitTypeId(motionActor))wc3::RemoveUnit(motionActor);
            auto hero=wc3::CreateUnit(wc3::GetLocalPlayer(),type,&x,&y,&facing);
            if(command=="actor")motionActor=hero; // Previous test bodies must not become movement obstacles.
            if(command=="arthas") {
                // Only the opt-in disposable oracle learns stock Arthas spells; normal maps remain untouched.
                using Level=void (__cdecl*)(wc3::Handle,int,BOOL);using Learn=void (__cdecl*)(wc3::Handle,int);
                reinterpret_cast<Level>(base+0x3C6D70)(hero,10,FALSE);
                for(int id:{0x41486862,0x41486473,0x41486164,0x41487265})reinterpret_cast<Learn>(base+0x3C7050)(hero,id);
                Mana(base,hero,1000);
            }
            using Clear=void (__cdecl*)();using Select=void (__cdecl*)(wc3::Handle,BOOL);
            reinterpret_cast<Clear>(base+0x3BBAA0)();reinterpret_cast<Select>(base+0x3C7910)(hero,TRUE);
            wc3::Log("fixture hero=%08X attack=%.1f",hero,wc3::UnitAttackAverage(hero));
        } else if (command=="backpack") {
            // Stock footmen already have Aihn; its slots unlock only after the player's Backpack research.
            using Tech=void (__cdecl*)(wc3::Handle,int,int);
            reinterpret_cast<Tech>(base+0x3C9730)(wc3::GetLocalPlayer(),0x5268706D,1);
            using Add=BOOL (__cdecl*)(wc3::Handle,int);
            BOOL added=reinterpret_cast<Add>(base+0x3C82A0)(unit,0x4169686E);
            wc3::Log("fixture backpack added=%d slots=%d",added,wc3::UnitInventorySize(unit));
        } else if (command=="party") {
            // Nearby owned footmen are eligible; allied/enemy owners, buildings, paused/dead and distant units are not.
            float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit)),facing=180;
            auto spawn=[&](const char* role,wc3::Handle owner,int type,float dx,float dy) {
                float ux=x+dx,uy=y+dy;auto created=wc3::CreateUnit(owner,type,&ux,&uy,&facing);party.push_back(created);
                wc3::Log("fixture party role=%s unit=%08X owner=%08X",role,created,owner);return created;
            };
            auto own=wc3::GetLocalPlayer();
            auto follower=spawn("owned1",own,0x68666F6F,-150,100);spawn("owned2",own,0x68666F6F,-150,-100);
            // Make the foreign-owner fixture genuinely allied, then place it outside the fight's acquisition radius.
            using Alliance=void (__cdecl*)(wc3::Handle,wc3::Handle,int,BOOL);
            auto alliance=reinterpret_cast<Alliance>(base+0x3C1050);
            alliance(own,wc3::Player(1),0,TRUE);alliance(wc3::Player(1),own,0,TRUE);
            spawn("ally",wc3::Player(1),0x68666F6F,350,-350);
            auto paused=spawn("paused",own,0x68666F6F,-250,100);wc3::PauseUnit(paused,TRUE);
            auto dead=spawn("dead",own,0x68666F6F,-250,-100);wc3::KillUnit(dead);
            spawn("building",own,0x68686F75,-300,0);spawn("distant",own,0x68666F6F,1100,0);
            enemy=spawn("enemy",wc3::Player(12),0x68666F6F,-150,190);
            wc3::IssueTargetOrderById(enemy,851983,follower);
        } else if (command=="step") {
            // Native placement moves only the private test leader; followers must use actual pathfinding to catch up.
            float dx=0,dy=0;input>>dx>>dy;
            float x=wc3::Real(wc3::GetUnitX(unit))+dx,y=wc3::Real(wc3::GetUnitY(unit))+dy;wc3::SetUnitPosition(unit,&x,&y);
        } else if (command=="enemy-remove") {
            if (enemy && wc3::GetUnitTypeId(enemy)) wc3::RemoveUnit(enemy);enemy=0;
        } else if (command=="drain") controller.TestDrainAmmo();
        else if (command=="rune" || command=="item") {
            std::string type;float dx=70,dy=0;input>>type>>dx>>dy;
            if (type.size()==4) {
                int id=0;for (char c:type) id=(id<<8)|static_cast<unsigned char>(c);
                float x=wc3::Real(wc3::GetUnitX(unit))+dx,y=wc3::Real(wc3::GetUnitY(unit))+dy;
                auto item=wc3::CreateItem(id,&x,&y);items.push_back(item);
                wc3::Log("fixture item=%08X type=%08X category=%d x=%.1f y=%.1f",item,id,wc3::GetItemType(item),x,y);
            }
        } else if (command=="enemy") {
            // A distant attacker reproduces pursuit as well as close retaliation in the disposable scene.
            float distance=100;int paused=0,owner=12;input>>distance>>paused>>owner;
            float x=wc3::Real(wc3::GetUnitX(unit))+distance,y=wc3::Real(wc3::GetUnitY(unit)),facing=180;
            enemy=wc3::CreateUnit(owner==-1 ? wc3::GetLocalPlayer() : wc3::Player(owner),0x68666F6F,&x,&y,&facing);
            using Order=BOOL (__cdecl*)(wc3::Handle,int,wc3::Handle);
            auto order=reinterpret_cast<Order>(base+0x3C89D0);
            // Native melee retaliation tries to autoattack/chase; FPS must leave the actor stationary and enemy undamaged.
            wc3::Log("fixture attack enemyOrder=%d",order(enemy,851983,unit));
            wc3::Log("fixture attack actorOrder=%d",order(unit,851983,enemy));
            if(paused)wc3::PauseUnit(enemy,TRUE); // Stable near/far targets isolate the read-only reach indicator.
        } else if(BackgroundGameplay::Enabled() && command=="aimenemy" && enemy) {
            // Aim through the production reticle mailbox at the fixture body's waist, without desktop input.
            float x=wc3::Real(wc3::GetUnitX(enemy)),y=wc3::Real(wc3::GetUnitY(enemy));
            std::ofstream request(std::string(root)+"\\test-target.request");request<<"aim "<<x<<" "<<y<<" "<<wc3::Ground(x,y)+60;
        } else if (command=="cleanup") {
            for (auto item:items) if (wc3::GetItemTypeId(item)) wc3::RemoveItem(item);
            items.clear();if (enemy && wc3::GetUnitTypeId(enemy)) wc3::RemoveUnit(enemy);enemy=0;
            for (auto actor:party) if (wc3::GetUnitTypeId(actor)) wc3::RemoveUnit(actor);party.clear();
        }
        observeUntil=check+5500;
    }
    if (check<observeUntil) {
        // Read-only telemetry proves real item consumption, all/current recovery and native AI effects.
        wc3::Log("fixture actor x=%.1f y=%.1f hp=%.1f attack=%.1f order=%d enemyHP=%.1f",
            wc3::Real(wc3::GetUnitX(unit)),wc3::Real(wc3::GetUnitY(unit)),wc3::Real(wc3::GetUnitState(unit,0)),
            wc3::UnitAttackAverage(unit),wc3::GetUnitCurrentOrder(unit),enemy ? wc3::Real(wc3::GetUnitState(enemy,0)) : -1);
        controller.TestLogAmmo();
        // Prediction must track reachable contacts without producing native attack events or damage.
        const auto& aim=controller.AimTarget();
        wc3::Log("fixture aim target=%08X enemy=%08X object=%d distance=%.1f hostile=%d circleX=%.1f circleZ=%.1f",aim.handle,enemy,aim.destructable,aim.distance,
            aim.enemy,aim.circle[0][0],aim.circle[0][2]);
        const auto& status=controller.Status();const auto& settings=controller.Settings();
        wc3::Log("fixture scaling attackSpeed=%.3f nativeMove=%.1f reference=%.1f fireEnabled=%d moveEnabled=%d damageMode=%d",
            status.attackSpeed,status.nativeSpeed,settings.movement.unitMoveSpeedReference,
            settings.scaleFireRateFromAttackSpeed,settings.movement.scaleFromUnitMoveSpeed,settings.heroDamage);
        controller.TestLogSquad();
        int slots=wc3::UnitInventorySize(unit),used=0;
        for (int i=0;i<slots;++i) if (wc3::UnitItemInSlot(unit,i)) ++used;
        wc3::Log("fixture inventory slots=%d used=%d squad=%u passive=%d",slots,used,unsigned(controller.SquadCount()),controller.SquadPassive());
    }
}
