#include "../src/BuyInventory.hpp"
#include "../src/BuyMenu.hpp"
#include "../src/BuyMenuLayout.hpp"
#include <cassert>

int main() {
    GameplaySettings settings;BuyInventory inventory;
    int ammo[7]={},reserve[7]={},magazines[]={30,30,12,10,0,1,0},reserves[]={90,90,100,30,0,0,0};
    inventory.Reset(settings,ammo,reserve,magazines,reserves);
    assert(!inventory.owned[0] && !inventory.owned[3] && inventory.owned[2] && inventory.owned[4] && inventory.owned[5] && inventory.owned[6]);
    assert(ammo[2]==12 && reserve[2]==100 && ammo[5]==20 && ammo[0]==0 && reserve[0]==0);
    assert(inventory.Next(4,1)==5 && inventory.Next(4,-1)==2);
    // Insufficient funds, duplicate guns and full reserves never debit the Warcraft resource budget.
    int gold=624;
    assert(inventory.Buy(0,false,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::NoGold);
    assert(gold==624 && ammo[0]==0 && !inventory.owned[0]);
    gold=1125;assert(inventory.Buy(0,false,settings,gold,ammo[0],reserve[0],30,90)==BuyInventory::Result::Bought);
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
    // Free refill never unlocks unowned guns, while F9 grants every slot without a gold transaction.
    settings.startAllWeapons=false;inventory.Reset(settings,ammo,reserve,magazines,reserves);
    ammo[2]=1;reserve[2]=0;gold=83;
    inventory.Refill(settings,ammo,reserve,magazines,reserves);
    assert(ammo[2]==12 && reserve[2]==100 && ammo[5]==settings.maxBombs && !inventory.owned[0] && ammo[0]==0 && gold==83);
    inventory.Refill(settings,ammo,reserve,magazines,reserves,true);
    for (int slot=0;slot<WeaponSlots::Count;++slot) assert(inventory.owned[slot]);
    assert(ammo[0]==30 && reserve[0]==90 && ammo[3]==10 && reserve[3]==30 && gold==83);
    // Every registered weapon is reachable in its menu; no duplicate keys or forgotten slots are permitted.
    bool seen[WeaponSlots::Count]={};
    for (const auto& item : BuyCatalog::Entries) {
        assert(item.slot>=0 && item.slot<WeaponSlots::Count && !seen[item.slot]);seen[item.slot]=true;
        assert(BuyCatalog::Find(item.category,item.key)==&item);
        BuyMenu menu;menu.Toggle();menu.Select(item.category==BuyCatalog::Category::Pistols ? 1 : item.category==BuyCatalog::Category::Rifles ? 4 : 8,2);
        assert(menu.Select(item.key,2).slot==item.slot);
    }
    for (bool present : seen) assert(present);
    BuyMenu menu;menu.Toggle();assert(menu.Select(2,2).slot==-1 && menu.Current()==BuyMenu::Page::Main);
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
