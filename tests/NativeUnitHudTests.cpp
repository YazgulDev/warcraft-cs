#include "../src/platform/NativeUnitHud.hpp"
#include <cassert>
#include <cstring>
#include <algorithm>

namespace {
int level=1,xp=125;bool hero=true;
int __cdecl Type(wc3::Handle actor) {return actor ? 123 : 0;}
wc3::Bits __cdecl State(wc3::Handle,int field) {float value[]={375,550,180,285};wc3::Bits bits;std::memcpy(&bits,&value[field],4);return bits;}
BOOL __cdecl Hero(wc3::Handle,int) {return hero;}
int __cdecl Level(wc3::Handle) {return level;}
int __cdecl XP(wc3::Handle) {return xp;}
int __cdecl Size(wc3::Handle) {return 6;}
wc3::Handle __cdecl Slot(wc3::Handle,int i) {return i==0 ? 77 : 0;}
int __cdecl Charges(wc3::Handle) {return 3;}
int __stdcall Threshold(int index) {int table[]={200,500,900};return table[std::clamp(index,1,3)-1];}
uintptr_t __fastcall Art(int,int) {return uintptr_t("map\\custom-item.blp");}
void Store(unsigned char* p,uintptr_t value) {std::memcpy(p,&value,4);}
void Jump(unsigned char* p,uintptr_t target) {p[0]=0xE9;Store(p+1,target-uintptr_t(p+5));}
}
namespace wc3 {
int (__cdecl* GetUnitTypeId)(Handle)=Type;
Bits (__cdecl* GetUnitState)(Handle,int)=State;
BOOL (__cdecl* IsUnitType)(Handle,int)=Hero;
int (__cdecl* GetHeroLevel)(Handle)=Level;
int (__cdecl* GetHeroXP)(Handle)=XP;
int (__cdecl* UnitInventorySize)(Handle)=Size;
Handle (__cdecl* UnitItemInSlot)(Handle,int)=Slot;
int (__cdecl* GetItemTypeId)(Handle)=Type;
int (__cdecl* GetItemCharges)(Handle)=Charges;
float Real(Bits bits) {float value;std::memcpy(&value,&bits,4);return value;}
}
void TestNativeUnitHud() {
    // Authored x86 thunks exercise signature validation and calling conventions without game code.
    auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,0xA00000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(memory);
    uintptr_t base=uintptr_t(memory);unsigned char threshold[]={0x56,0x8B,0x74,0x24,0x08,0x6A,0,0xBA};
    auto p=memory+0x207760;std::memcpy(p,threshold,8);Store(p+8,base+0x876208);
    unsigned char restore[]={0x83,0xC4,4,0x5E};std::memcpy(p+12,restore,4);Jump(p+16,uintptr_t(Threshold));
    p=memory+0x32DCB0;unsigned char art[]={0x56,0x8B,0xF2,0xBA};std::memcpy(p,art,4);Store(p+4,base+0x938FE0);
    p[8]=0x5E;Jump(p+9,uintptr_t(Art));FlushInstructionCache(GetCurrentProcess(),memory,0xA00000);
    NativeUnitHud reader;reader.Configure(base);auto data=reader.Read(1,true);
    assert(data.health==375 && data.maxHealth==550 && data.mana==180 && data.maxMana==285);
    assert(data.experienceKnown && data.levelExperience==0 && data.nextExperience==200 && data.experience==125);
    assert(data.inventorySize==6 && data.items[0].charges==3 && data.items[0].icon=="map\\custom-item.blp" && !data.items[1].type);
    level=2;xp=350;data=reader.Read(1,true);assert(data.levelExperience==200 && data.nextExperience==500);
    level=3;xp=500;data=reader.Read(1,true);assert(data.levelExperience==500 && data.nextExperience==500);
    hero=false;data=reader.Read(1,false);assert(!data.hero && !data.experienceKnown && !data.inventorySize);
    assert(!reader.Read(0,true).actor);
    hero=true;memory[0x207760]=0;memory[0x32DCB0]=0;reader.Configure(base);data=reader.Read(1,true);
    assert(!data.experienceKnown && data.items[0].icon.empty() && data.items[0].charges==3);
    VirtualFree(memory,0,MEM_RELEASE);
}
