#include "RuneCombatScene.hpp"
#include "BackgroundGameplay.hpp"
#include "FramePresentationScene.hpp"
#include "../src/platform/NativeSpellSlots.hpp"
#include "../src/platform/WeaponTargetFilter.hpp"
#include "../src/platform/UnitCollisionProfile.hpp"
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
    static wc3::Handle enemy=0,blocker=0,motionActor=0;static DWORD check=0,observeUntil=0;
    if ((!controller.Visible() && !BackgroundGameplay::Enabled()) || GetTickCount()-check<250) return;
    check=GetTickCount();auto unit=controller.TestUnit();
    std::string path=std::string(root)+"\\test-gameplay.request";
    if (GetFileAttributesA(path.c_str())!=INVALID_FILE_ATTRIBUTES) {
        std::ifstream file(path);std::string line;std::getline(file,line);file.close();DeleteFileA(path.c_str());
        std::istringstream input(line);std::string command;input>>command;
        // The background oracle uses the same controller mailboxes without sending desktop input.
        if(BackgroundGameplay::Enabled() && command=="toggle")controller.RequestToggle();
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
        // Exercise production wheel routing without injecting input into the user's desktop.
        else if(BackgroundGameplay::Enabled() && command=="wheel") {int delta=0;input>>delta;controller.RequestMouseWheel(delta);}
        else if(BackgroundGameplay::Enabled() && command=="heroattack")controller.TestHeroAttack();
        else if(BackgroundGameplay::Enabled() && command=="holdattack") {DWORD duration=0;input>>duration;controller.TestHoldAttack(duration);}
        else if(BackgroundGameplay::Enabled() && command=="targetstats")controller.TestLogTarget();
        else if(BackgroundGameplay::Enabled() && command=="pause") {int value=0;input>>value;wc3::PauseUnit(unit,value!=0);}
        else if(BackgroundGameplay::Enabled() && command=="camerawall") {
            // A tall, solid native building behind the actor probes boom collision without altering terrain.
            float x=wc3::Real(wc3::GetUnitX(unit))-220,y=wc3::Real(wc3::GetUnitY(unit)),facing=0;
            party.push_back(wc3::CreateUnit(wc3::GetLocalPlayer(),0x68746F77,&x,&y,&facing));
        }
        else if(BackgroundGameplay::Enabled() && command=="walk") {std::string key,gait;DWORD ms=0;input>>key>>ms>>gait;controller.TestWalk(ShortcutBindings::Parse(key),ms,gait=="slow");}
        else if(BackgroundGameplay::Enabled() && command=="motionstats") {
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
        else if(BackgroundGameplay::Enabled() && command=="buttons")Buttons(base);
        // New tuning follows production mailboxes; only disposable fixture builds expose these commands.
        else if(BackgroundGameplay::Enabled() && command=="settings")controller.RequestSettingsReload();
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
        else if(BackgroundGameplay::Enabled() && command=="strike") {
            // Aim/target creation uses the existing private request; this exercises production contact damage.
            int slot=2;input>>slot;controller.TestStrike(slot);
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
