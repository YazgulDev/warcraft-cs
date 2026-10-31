#include "../src/input/WeaponSelection.hpp"
#include "../src/input/WeaponWheel.hpp"
#include <limits>
#include <cassert>
#include <cstdio>

int main() {
    WeaponSelection menu;WeaponSelection::Available available={};
    available[WeaponSlots::Pistol]=available[WeaponSlots::Knife]=true;
    assert(!menu.Category(1,WeaponSlots::Pistol,available,0) && !menu.Open());
    assert(menu.Category(2,WeaponSlots::Pistol,available,0));
    assert(menu.Pending()==WeaponSlots::Pistol);
    assert(menu.Category(2,WeaponSlots::Pistol,available,1) && menu.Pending()==WeaponSlots::Pistol);
    menu.Close();assert(menu.Confirm(available)==-1);
    // Every catalog entry remains reachable without a limit on carried weapon types.
    available.fill(true);bool seen[WeaponSlots::Count]={};int total=0;
    for(int group=1;group<=WeaponSlots::SelectionGroups;++group) {
        auto items=WeaponSelection::Items(group,available);assert(!items.empty());menu.Close();
        for(size_t i=0;i<items.size();++i) {
            assert(menu.Category(group,-1,available,10));
            int slot=menu.Pending();assert(slot==items[i] && !seen[slot]);seen[slot]=true;++total;
        }
        assert(menu.Category(group,-1,available,11) && menu.Pending()==items.front());
    }
    assert(total==WeaponSlots::Count);
    // The wheel must visit the visible primary rows before crossing into the pistol group.
    menu.Close();menu.Cycle(2,0,available,19);
    assert(menu.Pending()==WeaponSlots::AWP);
    menu.Close();menu.Cycle(-1,0,available,20);assert(menu.Pending()==WeaponSlots::RPG);
    menu.Cycle(1,0,available,21);assert(menu.Pending()==0);
    menu.Cycle(4,0,available,22);assert(menu.Pending()==WeaponSlots::P90);
    assert(menu.Confirm(available)==WeaponSlots::P90 && !menu.Open());
    // Expected visible rows, including every category boundary; reversing retraces each individual step.
    const int ordered[]={0,1,WeaponSlots::AWP,WeaponSlots::MP5,WeaponSlots::P90,WeaponSlots::M249,
        WeaponSlots::M3,WeaponSlots::XM1014,WeaponSlots::TMP,WeaponSlots::UMP45,WeaponSlots::FAMAS,WeaponSlots::Galil,WeaponSlots::G3SG1,WeaponSlots::Pistol,WeaponSlots::Deagle,WeaponSlots::Glock,
        WeaponSlots::Knife,WeaponSlots::Sword,WeaponSlots::HE,WeaponSlots::Flashbang,WeaponSlots::Smoke,WeaponSlots::C4,
        WeaponSlots::Crowbar,WeaponSlots::Tau,WeaponSlots::Gluon,WeaponSlots::RPG};
    for(int i=0;i<WeaponSlots::Count;++i) {
        menu.Close();menu.Cycle(1,ordered[i],available,23);
        assert(menu.Pending()==ordered[(i+1)%WeaponSlots::Count]);
        menu.Cycle(-1,0,available,24);assert(menu.Pending()==ordered[i]);
    }
    // Feed actual wheel batches into selection; nine detents and complete inventory laps must not alias.
    WeaponWheel wheel;
    for(int detents:{-40,-17,-9,-1,1,9,17,40}) {
        menu.Close();wheel.Add(-120*detents);menu.Cycle(wheel.TakeSteps(),0,available,25);
        assert(menu.Pending()==ordered[(detents%WeaponSlots::Count+WeaponSlots::Count)%WeaponSlots::Count]);
        assert(wheel.TakeSteps()==0);
    }
    available.fill(false);available[0]=available[WeaponSlots::Pistol]=available[WeaponSlots::C4]=true;
    menu.Close();wheel.Add(-120*17);menu.Cycle(wheel.TakeSteps(),0,available,26);
    assert(menu.Pending()==WeaponSlots::C4); // 17 clicks traverse five full three-weapon laps, then two steps.
    menu.Close();menu.Cycle(1,WeaponSlots::Smoke,available,27);assert(menu.Pending()==WeaponSlots::C4);
    menu.Close();menu.Cycle(-1,WeaponSlots::Smoke,available,27);assert(menu.Pending()==WeaponSlots::Pistol);
    menu.Close();menu.Cycle(std::numeric_limits<long long>::min(),0,available,28);assert(menu.Pending()==WeaponSlots::Pistol);
    available.fill(true);
    // Browsing and timeout never mutate the equipped slot; expired/depleted choices cannot confirm.
    int equipped=WeaponSlots::Pistol;menu.Category(1,equipped,available,30);
    assert(equipped==WeaponSlots::Pistol && menu.Pending()==0);
    menu.Expire(34.9,5,available);assert(menu.Open());
    menu.Expire(35,5,available);assert(!menu.Open() && menu.Confirm(available)==-1);
    menu.Category(4,equipped,available,40);available[menu.Pending()]=false;
    assert(menu.Confirm(available)==-1);
    menu.Category(1,equipped,available,40);menu.Expire(39,5,available);assert(!menu.Open());
    available.fill(false);available[WeaponSlots::Deagle]=true;
    menu.Cycle(8,0,available,50);assert(menu.Pending()==WeaponSlots::Deagle);
    available.fill(false);menu.Cycle(1,0,available,51);assert(!menu.Open());
    assert(!menu.Category(0,0,available,52) && !menu.Category(6,0,available,52));
    std::puts("PASS owned category selection, all weapons, repeated keys, wheel wrap, confirmation, timeout and depletion.");
}
