#include "../src/combat/HalfLifeEnergy.hpp"
#include "../src/economy/BuyInventory.hpp"
#include "../src/input/WeaponSelection.hpp"
#include <cassert>
#include <cstdio>

int main() {
    HalfLifeSettings settings;HalfLifeEnergy tau;int cells=100;
    auto event=tau.Step(WeaponSlots::Tau,0,true,true,false,cells,.2f,settings);
    assert(event.shot && !event.charged && cells==98);
    assert(!tau.Step(WeaponSlots::Tau,.1,true,true,false,cells,.2f,settings).shot && cells==98);
    assert(!tau.Step(WeaponSlots::Tau,.3,false,true,false,cells,.2f,settings).shot); // Primary remains semi-auto.
    event=tau.Step(WeaponSlots::Tau,1,false,false,true,cells,.2f,settings);assert(event.started && cells==97);
    tau.Step(WeaponSlots::Tau,3,false,false,true,cells,.2f,settings);assert(cells==91);
    event=tau.Step(WeaponSlots::Tau,3.01,false,false,false,cells,.2f,settings);
    assert(event.shot && event.charged && event.charge>.5f && event.charge<.51f && !tau.Active());
    assert(!tau.Step(WeaponSlots::Tau,3.1,false,false,false,cells,.2f,settings).shot);
    // Losing context cancels, rather than firing a charged shot into a new menu/target/view.
    tau.Step(WeaponSlots::Tau,4,false,false,true,cells,.2f,settings);tau.Cancel();int saved=cells;
    assert(!tau.Step(WeaponSlots::Tau,8,false,false,false,cells,.2f,settings).shot && cells==saved);
    cells=1;HalfLifeEnergy empty;
    assert(!empty.Step(WeaponSlots::Tau,0,true,true,false,cells,.2f,settings).shot && cells==1);
    empty.Step(WeaponSlots::Tau,1,false,false,true,cells,.2f,settings);
    assert(empty.Step(WeaponSlots::Tau,1.01,false,false,true,cells,.2f,settings).shot && cells==0);
    HalfLifeEnergy hitch;cells=1;hitch.Step(WeaponSlots::Tau,0,false,false,true,cells,.2f,settings);
    event=hitch.Step(WeaponSlots::Tau,9,false,false,false,cells,.2f,settings);
    assert(event.shot && event.charge<.08f && cells==0); // A stalled frame cannot turn one cell into full charge.
    HalfLifeEnergy gluon;cells=3;
    assert(gluon.Step(WeaponSlots::Gluon,0,true,true,false,cells,.1f,settings).started && cells==2);
    assert(!gluon.Step(WeaponSlots::Gluon,.05,false,true,false,cells,.1f,settings).shot && cells==2);
    assert(gluon.Step(WeaponSlots::Gluon,.11,false,true,false,cells,.1f,settings).shot && cells==1);
    assert(gluon.Step(WeaponSlots::Gluon,.12,false,false,false,cells,.1f,settings).stopped && !gluon.Active());
    gluon.Step(WeaponSlots::Gluon,1,false,true,false,cells,.1f,settings);assert(cells==0);
    assert(gluon.Step(WeaponSlots::Gluon,1.2,false,true,false,cells,.1f,settings).stopped && cells==0);
    // Independently unavailable resources never unlock via purchases, grants or category selection.
    GameplaySettings game;BuyInventory inventory;int ammo[WeaponSlots::Count]={},reserve[WeaponSlots::Count]={};
    int magazines[WeaponSlots::Count]={},reserves[WeaponSlots::Count]={};
    for(int i=0;i<WeaponSlots::Count;++i)magazines[i]=game.weapons[i].magazine;
    for(int slot=WeaponSlots::Crowbar;slot<WeaponSlots::Count;++slot) {
        int gold=100000;assert(inventory.Buy(slot,false,game,gold,ammo[slot],reserve[slot],magazines[slot],0)==BuyInventory::Result::Invalid);
        game.halfLife.available[slot-WeaponSlots::Crowbar]=true;
        inventory.Reset(game,ammo,reserve,magazines,reserves);assert(!inventory.owned[slot]);
        assert(inventory.Buy(slot,false,game,gold,ammo[slot],reserve[slot],magazines[slot],0)==BuyInventory::Result::Bought);
        assert(gold==100000-game.weaponPrice[slot] && reserve[slot]==0);
        if(WeaponSlots::Energy(slot)) {
            int before=gold;assert(inventory.Buy(slot,true,game,gold,ammo[slot],reserve[slot],100,0)==BuyInventory::Result::Full && gold==before);
            ammo[slot]=95;assert(inventory.Buy(slot,true,game,gold,ammo[slot],reserve[slot],100,0)==BuyInventory::Result::Bought);
            assert(ammo[slot]==100 && reserve[slot]==0 && gold==before-game.ammoPrice[slot]);
        }
        game.halfLife.enabled[slot-WeaponSlots::Crowbar]=false;assert(!game.WeaponEnabled(slot));
        game.halfLife.enabled[slot-WeaponSlots::Crowbar]=true;
        game.warcraftOnly=true;assert(!game.WeaponEnabled(slot));game.warcraftOnly=false;
    }
    std::puts("PASS Tau primary/charge/cancel/exhaustion, Gluon cadence/release, independent optional economy and direct-cell refill.");
}
