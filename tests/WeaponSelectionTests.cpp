#include "../src/input/WeaponSelection.hpp"
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
    for(int group=1;group<=5;++group) {
        auto items=WeaponSelection::Items(group,available);assert(!items.empty());menu.Close();
        for(size_t i=0;i<items.size();++i) {
            assert(menu.Category(group,-1,available,10));
            int slot=menu.Pending();assert(slot==items[i] && !seen[slot]);seen[slot]=true;++total;
        }
        assert(menu.Category(group,-1,available,11) && menu.Pending()==items.front());
    }
    assert(total==WeaponSlots::Count);
    menu.Close();menu.Cycle(-1,0,available,20);assert(menu.Pending()==WeaponSlots::Smoke);
    menu.Cycle(1,0,available,21);assert(menu.Pending()==0);
    menu.Cycle(4,0,available,22);assert(menu.Pending()==WeaponSlots::Knife);
    assert(menu.Confirm(available)==WeaponSlots::Knife && !menu.Open());
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
