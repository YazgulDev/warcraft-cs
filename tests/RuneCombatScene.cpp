#include "RuneCombatScene.hpp"
#include <fstream>
#include <sstream>
#include <vector>

void RuneCombatScene::Tick(ShooterController& controller,uintptr_t base,const char* root) {
    static std::vector<wc3::Handle> items;
    static std::vector<wc3::Handle> party;
    static wc3::Handle enemy=0;static DWORD check=0,observeUntil=0;
    if (!controller.Visible() || GetTickCount()-check<250) return;
    check=GetTickCount();auto unit=controller.TestUnit();
    std::string path=std::string(root)+"\\test-gameplay.request";
    if (GetFileAttributesA(path.c_str())!=INVALID_FILE_ATTRIBUTES) {
        std::ifstream file(path);std::string line;std::getline(file,line);file.close();DeleteFileA(path.c_str());
        std::istringstream input(line);std::string command;input>>command;
        if (command=="hero" || command=="creep") {
            // A native hero has an inventory, unlike an unupgraded starting worker.
            // Move the private actor away from the starting workers so squad membership has an exact oracle.
            float x=wc3::Real(wc3::GetUnitX(unit))-1000,y=wc3::Real(wc3::GetUnitY(unit)),facing=0;
            auto hero=wc3::CreateUnit(wc3::GetLocalPlayer(),command=="hero" ? 0x4870616C : 0x68666F6F,&x,&y,&facing);
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
            float x=wc3::Real(wc3::GetUnitX(unit))+100,y=wc3::Real(wc3::GetUnitY(unit)),facing=180;
            enemy=wc3::CreateUnit(wc3::Player(12),0x68666F6F,&x,&y,&facing);
            using Order=BOOL (__cdecl*)(wc3::Handle,int,wc3::Handle);
            auto order=reinterpret_cast<Order>(base+0x3C89D0);
            // Native melee retaliation tries to autoattack/chase; FPS must leave the actor stationary and enemy undamaged.
            wc3::Log("fixture attack enemyOrder=%d",order(enemy,851983,unit));
            wc3::Log("fixture attack actorOrder=%d",order(unit,851983,enemy));
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
        controller.TestLogSquad();
        int slots=wc3::UnitInventorySize(unit),used=0;
        for (int i=0;i<slots;++i) if (wc3::UnitItemInSlot(unit,i)) ++used;
        wc3::Log("fixture inventory slots=%d used=%d squad=%u passive=%d",slots,used,unsigned(controller.SquadCount()),controller.SquadPassive());
    }
}
