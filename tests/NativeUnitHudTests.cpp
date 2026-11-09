#include "../src/platform/NativeUnitHud.hpp"
#include <cassert>
#include <cstring>
#include <algorithm>

namespace {
int level=1,xp=125,maxLevel=3;bool hero=true;
int __cdecl Type(wc3::Handle actor) {return actor ? 123 : 0;}
wc3::Bits __cdecl State(wc3::Handle,int field) {float value[]={375,550,180,285};wc3::Bits bits;std::memcpy(&bits,&value[field],4);return bits;}
BOOL __cdecl Hero(wc3::Handle,int) {return hero;}
int __cdecl Level(wc3::Handle) {return level;}
int __cdecl XP(wc3::Handle) {return xp;}
int __cdecl Size(wc3::Handle) {return 6;}
wc3::Handle __cdecl Slot(wc3::Handle,int i) {return i==0 ? 77 : 0;}
int __cdecl Charges(wc3::Handle) {return 3;}
// Warcraft repeats the previous entry at the cap, so equality also occurs one level early.
int __stdcall Threshold(int index) {
    const int table[]={200,500,900,1400};
    return maxLevel==1 ? 0 : table[std::clamp(index,1,maxLevel-1)-1];
}
int __fastcall Constant(const char* table,const char* key,int index) {
    assert(!std::strcmp(table,"test-map") && !std::strcmp(key,"MaxHeroLevel") && index==0);
    return maxLevel;
}
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
    p[12]=0xB9;Store(p+13,base+0x875DE0);
    unsigned char restore[]={0x83,0xC4,4,0x5E};std::memcpy(p+17,restore,4);Jump(p+21,uintptr_t(Threshold));
    std::strcpy(reinterpret_cast<char*>(memory+0x875DE0),"test-map");
    std::strcpy(reinterpret_cast<char*>(memory+0x876208),"MaxHeroLevel");
    p=memory+0x009F90;
    unsigned char constant[]={0x51,0x53,0x55,0x56,0x8B,0xD9,0x57,0x5F,0x5E,0x5D,0x5B,0x59};
    std::memcpy(p,constant,sizeof(constant));Jump(p+sizeof(constant),uintptr_t(Constant));
    p=memory+0x32DCB0;unsigned char art[]={0x56,0x8B,0xF2,0xBA};std::memcpy(p,art,4);Store(p+4,base+0x938FE0);
    p[8]=0x5E;Jump(p+9,uintptr_t(Art));FlushInstructionCache(GetCurrentProcess(),memory,0xA00000);
    NativeUnitHud reader;reader.Configure(base);auto data=reader.Read(1,true);
    assert(data.health==375 && data.maxHealth==550 && data.mana==180 && data.maxMana==285);
    assert(data.experienceKnown && data.levelExperience==0 && data.nextExperience==200 && data.experience==125);
    assert(data.inventorySize==6 && data.items[0].charges==3 && data.items[0].icon=="map\\custom-item.blp" && !data.items[1].type);
    level=2;xp=350;data=reader.Read(1,true);assert(data.experienceKnown && data.levelExperience==200 && data.nextExperience==500);
    level=3;xp=500;data=reader.Read(1,true);assert(data.experienceKnown && data.levelExperience==500 && data.nextExperience==500);
    maxLevel=5;data=reader.Read(1,true);assert(data.experienceKnown && data.nextExperience==900);
    maxLevel=1;level=1;data=reader.Read(1,true);assert(data.experienceKnown && data.levelExperience==0 && data.nextExperience==0);
    hero=false;data=reader.Read(1,false);assert(!data.hero && !data.experienceKnown && !data.inventorySize);
    assert(!reader.Read(0,true).actor);
    hero=true;memory[0x207760]=0;memory[0x32DCB0]=0;reader.Configure(base);data=reader.Read(1,true);
    assert(!data.experienceKnown && data.items[0].icon.empty() && data.items[0].charges==3);
    VirtualFree(memory,0,MEM_RELEASE);
}
