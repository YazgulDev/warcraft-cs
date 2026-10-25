#include "../src/economy/DollarWallet.hpp"
#include "../src/economy/CurrencyDisplay.hpp"
#include "../src/economy/BuyInventory.hpp"
#include "../src/inventory/WeaponDropRules.hpp"
#include "../src/platform/DroppedWeapons.hpp"
#include "../src/platform/KillReward.hpp"
#include <cassert>
#include <cstring>

namespace wc3 {
#define NATIVE(result,name,args,offset) result (__cdecl* name) args=nullptr;
#include "../src/platform/NativeOffsets.inc"
#undef NATIVE
float Real(Bits bits){float result;std::memcpy(&result,&bits,4);return result;}
static Bits BitsOf(float value){Bits result;std::memcpy(&result,&value,4);return result;}
int effects=0,destroyed=0;bool effectAvailable=true;
Handle Effect(const char*,float,float){return effectAvailable ? Handle(++effects) : 0;}
}
static float clockTime=0,health=100;
static bool enemy=true,building=false,present=true;
int main() {
    GameplaySettings settings;DollarWallet wallet;BuyInventory inventory;
    assert(wallet.Balance(1,settings.economy)==800 && wallet.Balance(2,settings.economy)==800);
    int balance=wallet.Balance(1,settings.economy),ammo=0,reserve=0;
    assert(inventory.Buy(0,false,settings,balance,ammo,reserve,30,90)==BuyInventory::Result::NoGold && balance==800);
    // Three $200 rewards raise the $800 start enough to buy the $1250 rifle.
    assert(wallet.Reward(1,settings.economy)==200);
    wallet.Reward(1,settings.economy);wallet.Reward(1,settings.economy);balance=wallet.Balance(1,settings.economy);
    assert(inventory.Buy(0,false,settings,balance,ammo,reserve,30,90)==BuyInventory::Result::Bought);
    wallet.Store(1,balance,settings.economy);assert(wallet.Balance(1,settings.economy)==150 && wallet.Balance(2,settings.economy)==800);
    // Reloads, unit switches and the money cap never recreate starting funds.
    settings.economy.startingMoney=9000;assert(wallet.Balance(1,settings.economy)==150);
    wallet.Store(1,15950,settings.economy);assert(wallet.Reward(1,settings.economy)==50 && wallet.Reward(1,settings.economy)==0);
    settings.economy.maximumMoney=1000;assert(wallet.Balance(1,settings.economy)==1000);
    wallet.Reset();settings.economy=EconomySettings{};assert(wallet.Balance(1,settings.economy)==800);
    assert(CurrencyDisplay::Format(123,settings.economy)=="$123");settings.economy.currency=EconomySettings::Currency::Gold;
    assert(CurrencyDisplay::Format(123,settings.economy)=="123 GOLD");
    // One loaded weapon, capped duplicate ammo, no consumption when full, and no optional asset bypass.
    BuyInventory drops;ammo=reserve=0;assert(drops.Pickup(0,settings,ammo,reserve) && ammo==30 && reserve==0);
    assert(drops.Pickup(0,settings,ammo,reserve) && reserve==30);reserve=89;
    assert(drops.Pickup(0,settings,ammo,reserve) && reserve==90);assert(!drops.Pickup(0,settings,ammo,reserve));
    assert(!drops.Pickup(WeaponSlots::RPG,settings,ammo,reserve));
    auto candidates=WeaponDropRules::Candidates(settings);for(int slot:candidates)assert(WeaponSlots::Firearm(slot) && slot!=WeaponSlots::RPG);
    settings.rpg.available=true;assert(WeaponDropRules::Candidates(settings).back()==WeaponSlots::RPG);
    // The unified arsenal uses usable energy cells for duplicate drops, with no invisible reserve pool.
    settings.halfLife.available[1]=settings.halfLife.available[2]=true;
    ammo=reserve=0;assert(drops.Pickup(WeaponSlots::Tau,settings,ammo,reserve));
    ammo=95;assert(drops.Pickup(WeaponSlots::Tau,settings,ammo,reserve) && ammo==100 && reserve==0);
    assert(!drops.Pickup(WeaponSlots::Tau,settings,ammo,reserve));
    settings.warcraftOnly=true;assert(WeaponDropRules::Candidates(settings).empty());settings.warcraftOnly=false;
    // Exhaust the probability boundary without a flaky random sample: .5 percent admits 500 / 100000 buckets.
    int accepted=0;for(int i=0;i<100000;++i)accepted+=WeaponDropRules::Success(settings.weaponDrops,float(i)/1000);
    assert(accepted==500);settings.weaponDrops.enabled=false;assert(!WeaponDropRules::Success(settings.weaponDrops,0));
    settings.weaponDrops.enabled=true;settings.weaponDrops.chancePercent=0;assert(!WeaponDropRules::Success(settings.weaponDrops,0));
    settings.weaponDrops.chancePercent=100;assert(WeaponDropRules::Success(settings.weaponDrops,99.999f));

    wc3::CreateTimer=[]()->wc3::Handle{return 1;};wc3::TimerStart=[](wc3::Handle,float*,BOOL,wc3::Handle){};
    wc3::TimerGetElapsed=[](wc3::Handle){return wc3::BitsOf(clockTime);};
    wc3::GetUnitTypeId=[](wc3::Handle u){return u==2 && !present ? 0 : int(u);};
    wc3::GetUnitState=[](wc3::Handle,int){return wc3::BitsOf(health);};
    wc3::GetOwningPlayer=[](wc3::Handle)->wc3::Handle{return 1;};
    wc3::IsUnitEnemy=[](wc3::Handle,wc3::Handle)->BOOL{return enemy;};
    wc3::IsUnitType=[](wc3::Handle,int)->BOOL{return building;};
    wc3::GetUnitX=[](wc3::Handle){return wc3::BitsOf(0);};wc3::GetUnitY=wc3::GetUnitX;
    wc3::DestroyEffect=[](wc3::Handle){++wc3::destroyed;};
    int rewards=0;KillReward::Sink sink=[&](const KillReward& event){assert(event.owner==1);++rewards;};
    auto hit=KillReward::Capture(1,2,sink);hit.Confirm(sink);assert(rewards==0);
    health=0;hit.Confirm(sink);assert(rewards==1);
    KillReward::Capture(1,2,sink).Confirm(sink);assert(rewards==1);
    health=100;enemy=false;auto allied=KillReward::Capture(1,2,sink);health=0;allied.Confirm(sink);assert(rewards==1);
    health=100;enemy=true;building=true;auto structure=KillReward::Capture(1,2,sink);health=0;structure.Confirm(sink);assert(rewards==1);
    health=100;building=false;auto removed=KillReward::Capture(1,2,sink);present=false;removed.Confirm(sink);assert(rewards==2);present=true;
    // Ground loot expires with the simulation clock, survives pauses/full magazines and clears without stale map calls.
    DroppedWeapons field;settings.weaponDrops=WeaponDropSettings{};settings.weaponDrops.maximumDrops=1;
    assert(field.Spawn(0,30,0,settings.weaponDrops));assert(!field.Spawn(1,30,0,settings.weaponDrops));
    assert(field.Nearest(1,20)==-1 && field.Nearest(1,40)==0 && field.Slot(0)==0);
    clockTime=119;field.Tick(settings.weaponDrops,true);assert(field.Nearest(1,40)==0);
    field.Tick(settings.weaponDrops,true);assert(wc3::destroyed==0);clockTime=120;field.Tick(settings.weaponDrops,true);assert(wc3::destroyed==1 && field.Nearest(1,40)==-1);
    assert(field.Spawn(0,0,0,settings.weaponDrops));field.Tick(settings.weaponDrops,false);assert(wc3::destroyed==2);
    wc3::effectAvailable=false;assert(!field.Spawn(0,0,0,settings.weaponDrops));wc3::effectAvailable=true;
    assert(field.Spawn(0,0,0,settings.weaponDrops));field.Reset();assert(wc3::destroyed==2 && field.Nearest(1,40)==-1);
    return 0;
}
