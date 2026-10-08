#include "../src/BuyInventory.hpp"
#include "../src/BuyMenu.hpp"
#include "../src/BuyMenuLayout.hpp"
#include <cassert>

int main() {
    GameplaySettings settings;BuyInventory inventory;
    int ammo[7]={},reserve[7]={},magazines[]={30,30,12,10,0,1,0},reserves[]={90,90,100,30,0,0,0};
    inventory.Reset(settings,ammo,reserve,magazines,reserves);
    assert(!inventory.owned[0] && !inventory.owned[3] && inventory.owned[4] && inventory.owned[5] && inventory.owned[6]);
    assert(ammo[5]==20 && ammo[0]==0 && reserve[0]==0);
    assert(inventory.Next(4,1)==5 && inventory.Next(4,-1)==6);
    // Insufficient funds, duplicate guns and full reserves never debit the Warcraft resource budget.
    int gold=2499;
    assert(inventory.Buy(0,false,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::NoGold);
    assert(gold==2499 && ammo[0]==0 && !inventory.owned[0]);
    gold=3000;assert(inventory.Buy(0,false,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Bought);
    assert(gold==500 && ammo[0]==30 && reserve[0]==0 && inventory.owned[0]);
    assert(inventory.Buy(0,false,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::AlreadyOwned && gold==500);
    assert(inventory.Buy(1,true,settings,gold,ammo[1],reserve[1],30,90)==BuyInventory::Result::NotOwned && gold==500);
    assert(inventory.Buy(0,true,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Bought && gold==420 && reserve[0]==30);
    reserve[0]=89;assert(inventory.Buy(0,true,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Bought && reserve[0]==90);
    int before=gold;assert(inventory.Buy(0,true,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Full && gold==before);
    settings.weaponPrice[5]=7;gold=7;
    assert(inventory.Buy(5,false,settings,gold,ammo[5],reserve[5],1,0)==BuyInventory::Result::Bought && gold==0 && ammo[5]==21);
    settings.maxBombs=21;gold=100;
    assert(inventory.Buy(5,true,settings,gold,ammo[5],reserve[5],1,0)==BuyInventory::Result::Full && ammo[5]==21 && gold==100);
    settings.startAllWeapons=true;inventory.Reset(settings,ammo,reserve,magazines,reserves);
    assert(inventory.owned[0] && inventory.owned[3] && ammo[0]==30 && reserve[0]==90);
    // Category selections route purchases without also switching a weapon; back/main/close are deterministic.
    BuyMenu menu;menu.Toggle();auto action=menu.Select(2,4);assert(action.slot==-1 && menu.Current()==BuyMenu::Page::Rifles);
    assert(menu.Select(3,4).slot==3);menu.Select(0,4);assert(menu.Current()==BuyMenu::Page::Main);
    assert(menu.Select(5,2).ammo && menu.Select(5,2).slot==2);
    assert(menu.Select(6,2).allAmmo);menu.Select(0,2);assert(!menu.Open());
    for (const auto size : {std::pair<int,int>{1280,720},{1920,1080},{800,600}}) {
        BuyMenuLayout box(size.first,size.second);
        assert(BuyMenuLayout::Key(int(box.x+30),int(box.y+box.row*2.5f),size.first,size.second,BuyMenu::Page::Main)==1);
        assert(BuyMenuLayout::Key(int(box.x+30),int(box.y+box.row*8.5f),size.first,size.second,BuyMenu::Page::Main)==0);
        assert(BuyMenuLayout::Key(0,0,size.first,size.second,BuyMenu::Page::Main)==-1);
    }
}
