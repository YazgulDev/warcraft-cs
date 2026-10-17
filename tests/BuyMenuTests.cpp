#include "../src/economy/BuyInventory.hpp"
#include "../src/economy/BuyMenu.hpp"
#include "../src/presentation/BuyMenuLayout.hpp"
#include <cassert>

int main() {
    GameplaySettings settings;BuyInventory inventory;
    // Both C4 purchase paths buy one charge for the requested price.
    assert(settings.weaponPrice[WeaponSlots::C4]==3000 && settings.ammoPrice[WeaponSlots::C4]==3000);
    for(bool quickAmmo : {false,true}) {
        BuyInventory bombs;int gold=2999,count=0,reserve=0;bombs.owned[WeaponSlots::C4]=true;
        assert(bombs.Buy(WeaponSlots::C4,quickAmmo,settings,gold,count,reserve,1,0)==BuyInventory::Result::NoGold);
        assert(gold==2999 && count==0);
        gold=3000;assert(bombs.Buy(WeaponSlots::C4,quickAmmo,settings,gold,count,reserve,1,0)==BuyInventory::Result::Bought);
        assert(gold==0 && count==1);
    }
    int ammo[WeaponSlots::Count]={},reserve[WeaponSlots::Count]={},magazines[WeaponSlots::Count]={30,30,12,10,0,1,0,7,20,30,50,100,8,7},reserves[WeaponSlots::Count]={90,90,100,30,0,0,0,35,120,120,100,200,32,32};
    inventory.Reset(settings,ammo,reserve,magazines,reserves);
    assert(!inventory.owned[0] && !inventory.owned[3] && inventory.owned[2] && inventory.owned[4] && !inventory.owned[5] && inventory.owned[6]);
    assert(ammo[2]==12 && reserve[2]==100 && ammo[5]==0 && ammo[0]==0 && reserve[0]==0);
    assert(inventory.Next(4,1)==6 && inventory.Next(4,-1)==2);
    // Optional starting bombs still grant ownership; zero keeps C4 inaccessible until purchase/free grant.
    settings.startBombs=5;inventory.Reset(settings,ammo,reserve,magazines,reserves);
    assert(inventory.owned[5] && ammo[5]==5 && inventory.Next(4,1)==5);
    settings.startBombs=0;inventory.Reset(settings,ammo,reserve,magazines,reserves);
    // Insufficient funds, duplicate guns and full reserves never debit the Warcraft resource budget.
    int gold=1249;
    assert(inventory.Buy(0,false,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::NoGold);
    assert(gold==1249 && ammo[0]==0 && !inventory.owned[0]);
    gold=1750;assert(inventory.Buy(0,false,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Bought);
    assert(gold==500 && ammo[0]==30 && reserve[0]==0 && inventory.owned[0]);
    assert(inventory.Buy(0,false,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::AlreadyOwned && gold==500);
    assert(inventory.Buy(1,true,settings,gold,ammo[1],reserve[1],30,90)==BuyInventory::Result::NotOwned && gold==500);
    assert(inventory.Buy(0,true,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Bought && gold==420 && reserve[0]==30);
    reserve[0]=89;assert(inventory.Buy(0,true,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Bought && reserve[0]==90);
    int before=gold;assert(inventory.Buy(0,true,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Full && gold==before);
    settings.weaponPrice[5]=7;gold=7;
    assert(inventory.Buy(5,false,settings,gold,ammo[5],reserve[5],1,0)==BuyInventory::Result::Bought && gold==0 && ammo[5]==1 && inventory.owned[5]);
    settings.maxBombs=1;gold=100;
    settings.unlimitedBombPurchases=false;
    assert(inventory.Buy(5,true,settings,gold,ammo[5],reserve[5],1,0)==BuyInventory::Result::Full && ammo[5]==1 && gold==100);
    // Both purchase paths remain repeatable beyond MaxBombs, charging every useful transaction.
    settings.unlimitedBombPurchases=true;settings.weaponPrice[5]=settings.ammoPrice[5]=7;
    for(bool quickAmmo : {false,true}) {
        gold=21;int previous=ammo[5];
        for(int n=0;n<3;++n)assert(inventory.Buy(5,quickAmmo,settings,gold,ammo[5],reserve[5],1,0)==BuyInventory::Result::Bought);
        assert(gold==0 && ammo[5]==previous+3);
    }
    inventory.Refill(settings,ammo,reserve,magazines,reserves);assert(ammo[5]==7);
    int fullStorage=std::numeric_limits<int>::max();gold=7;
    assert(inventory.Buy(5,false,settings,gold,fullStorage,reserve[5],1,0)==BuyInventory::Result::Full && gold==7);
    settings.startAllWeapons=true;inventory.Reset(settings,ammo,reserve,magazines,reserves);
    assert(inventory.owned[0] && inventory.owned[3] && inventory.owned[5] && ammo[5]==0 && ammo[0]==30 && reserve[0]==90);
    // F7 grants C4 while leaving unowned firearms locked; F9 grants every slot without a gold transaction.
    settings.startAllWeapons=false;inventory.Reset(settings,ammo,reserve,magazines,reserves);
    ammo[2]=1;reserve[2]=0;gold=83;
    inventory.Refill(settings,ammo,reserve,magazines,reserves);
    assert(ammo[2]==12 && reserve[2]==100 && ammo[5]==settings.maxBombs && inventory.owned[5] && !inventory.owned[0] && ammo[0]==0 && gold==83);
    inventory.Refill(settings,ammo,reserve,magazines,reserves,true);
    for (int slot=0;slot<WeaponSlots::Count;++slot) assert(inventory.owned[slot]);
    assert(ammo[0]==30 && reserve[0]==90 && ammo[3]==10 && reserve[3]==30 && ammo[5]==settings.maxBombs && gold==83);
    // A custom capacity supports larger reserves and prevents charging gold when the configured cap is zero.
    settings.maxReserve[0]=250;reserves[0]=settings.maxReserve[0];
    inventory.Refill(settings,ammo,reserve,magazines,reserves);
    assert(reserve[0]==250 && ammo[0]==30);
    reserve[0]=240;gold=100;
    assert(inventory.Buy(0,true,settings,gold,ammo[0],reserve[0],30,settings.maxReserve[0])==BuyInventory::Result::Bought);
    assert(reserve[0]==250 && gold==20);
    settings.maxReserve[0]=0;reserve[0]=0;gold=100;
    assert(inventory.Buy(0,true,settings,gold,ammo[0],reserve[0],30,settings.maxReserve[0])==BuyInventory::Result::Full && gold==100);
    // Every registered weapon is reachable in its menu; no duplicate keys or forgotten slots are permitted.
    bool seen[WeaponSlots::Count]={};
    for (const auto& item : BuyCatalog::Entries) {
        assert(item.slot>=0 && item.slot<WeaponSlots::Count && !seen[item.slot]);seen[item.slot]=true;
        assert(BuyCatalog::Find(item.category,item.key)==&item);
        BuyMenu menu;menu.Toggle();menu.Select(item.category==BuyCatalog::Category::Pistols ? 1 : item.category==BuyCatalog::Category::Rifles ? 4 : item.category==BuyCatalog::Category::Shotguns ? 2 : item.category==BuyCatalog::Category::SMGs ? 3 : item.category==BuyCatalog::Category::Machineguns ? 5 : 8,2);
        assert(menu.Select(item.key,2).slot==item.slot);
    }
    for (bool present : seen) assert(present);
    // Appended firearms obey the same paid ownership, reload capacities and free-grant boundaries as the original slots.
    for(int slot=WeaponSlots::Deagle;slot<WeaponSlots::Count;++slot) {
        BuyInventory fresh;int wallet=settings.weaponPrice[slot],loaded=0,spare=0;
        assert(fresh.Buy(slot,false,settings,wallet,loaded,spare,magazines[slot],reserves[slot])==BuyInventory::Result::Bought);
        assert(wallet==0 && loaded==magazines[slot] && fresh.owned[slot] && spare==0);
        wallet=settings.ammoPrice[slot];
        assert(fresh.Buy(slot,true,settings,wallet,loaded,spare,magazines[slot],reserves[slot])==BuyInventory::Result::Bought);
        assert(wallet==0 && spare==settings.ammoPack[slot]);
    }
    BuyMenu menu;menu.Toggle();assert(menu.Select(9,2).slot==-1 && menu.Current()==BuyMenu::Page::Main);
    assert(menu.Select(6,2).primaryAmmo && menu.Select(7,2).secondaryAmmo);
    menu.Select(4,2);assert(menu.Select(3,2).slot==3);menu.Select(0,2);assert(menu.Current()==BuyMenu::Page::Main);
    menu.Select(0,2);assert(!menu.Open());
    // Sparse CS keys, row spacing, cancel and details all agree with the rendered hitboxes.
    for (const auto size : {std::pair<int,int>{1280,720},{1920,1080},{800,600}}) {
        BuyMenuLayout box(size.first,size.second);
        const auto keys=BuyMenuLayout::Keys(BuyMenu::Page::Main);
        for (size_t row=0;row<keys.size();++row) {
            assert(BuyMenuLayout::Key(int(box.listX+10),int(box.listY+(row+.3f)*box.row),size.first,size.second,BuyMenu::Page::Main)==keys[row]);
            assert(BuyMenuLayout::Key(int(box.listX+10),int(box.listY+(row+.8f)*box.row),size.first,size.second,BuyMenu::Page::Main)==-1);
        }
        assert(BuyMenuLayout::Key(int(box.listX+10),int(box.cancelY+box.row*.3f),size.first,size.second,BuyMenu::Page::Main)==0);
        assert(BuyMenuLayout::Key(int(box.detailX+10),int(box.previewY+10),size.first,size.second,BuyMenu::Page::Rifles)==-1);
        assert(BuyMenuLayout::Key(0,0,size.first,size.second,BuyMenu::Page::Main)==-1);
    }
}
