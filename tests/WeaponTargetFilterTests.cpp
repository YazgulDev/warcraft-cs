#include "../src/platform/WeaponTargetFilter.hpp"
#include "../src/platform/UnitCollisionProfile.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <limits>

namespace {
std::array<unsigned char,0x200> unit{},position{},profile{};
bool locust=false,hero=false,building=false,resolvable=true;
int __cdecl Ability(wc3::Handle,int type) {assert(type==0x416C6F63);return locust ? 1 : 0;}
BOOL __cdecl Type(wc3::Handle,int type) {return type==0 ? hero : type==2 ? building : false;}
uintptr_t __fastcall Position(unsigned index,unsigned generation) {
    assert(index==11 && generation==12);return resolvable ? reinterpret_cast<uintptr_t>(position.data()) : 0;
}
void Radius(float value) {std::memcpy(position.data()+0x90,&value,4);}
}
namespace wc3 {
int (__cdecl* GetUnitAbilityLevel)(Handle,int)=Ability;
BOOL (__cdecl* IsUnitType)(Handle,int)=Type;
uintptr_t UnitObject(Handle handle) {return handle ? reinterpret_cast<uintptr_t>(unit.data()) : 0;}
}
int main() {
    auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x40000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(memory);
    uintptr_t base=reinterpret_cast<uintptr_t>(memory),entry=base+0x3FA30;
    *reinterpret_cast<unsigned char*>(entry)=0xE9;
    *reinterpret_cast<int*>(entry+1)=int(reinterpret_cast<uintptr_t>(Position)-entry-5);
    *reinterpret_cast<unsigned*>(unit.data()+0x16C)=11;*reinterpret_cast<unsigned*>(unit.data()+0x170)=12;
    // Large cosmetic Locust bounds are transparent independently of their authored model radius.
    locust=true;Radius(100);assert(WeaponTargetFilter::PassThrough(1,base));locust=false;
    // Immunity alone never makes a physical unit transparent; vulnerable zero-collision units are still targets.
    Radius(0);assert(!WeaponTargetFilter::PassThrough(1,base));
    *reinterpret_cast<unsigned*>(unit.data()+0x20)=8;assert(WeaponTargetFilter::PassThrough(1,base));
    Radius(16);assert(!WeaponTargetFilter::PassThrough(1,base));
    // Disabling native pathing may clear the mask without changing the unit's authored radius.
    *reinterpret_cast<uintptr_t*>(position.data()+0xA8)=reinterpret_cast<uintptr_t>(profile.data());
    *reinterpret_cast<unsigned*>(profile.data()+0x9C)=0x1000000;assert(WeaponTargetFilter::PassThrough(1,base));
    *reinterpret_cast<unsigned*>(profile.data()+0x9C)=1;assert(!WeaponTargetFilter::PassThrough(1,base));
    // Movement uses live radius/mask regardless of immunity or visual model size; shooting keeps its separate policy.
    Radius(.5f);auto collision=UnitCollisionProfile::Read(1,base);assert(collision.known && collision.colliding && collision.radius==16);
    *reinterpret_cast<unsigned*>(profile.data()+0x9C)=0;
    collision=UnitCollisionProfile::Read(1,base);assert(collision.known && !collision.colliding);
    *reinterpret_cast<unsigned*>(profile.data()+0x9C)=1;Radius(0);
    collision=UnitCollisionProfile::Read(1,base);assert(collision.known && !collision.colliding);
    locust=true;collision=UnitCollisionProfile::Read(1,base);assert(collision.known && !collision.colliding);locust=false;
    Radius(0);hero=true;assert(!WeaponTargetFilter::PassThrough(1,base));hero=false;
    building=true;assert(!WeaponTargetFilter::PassThrough(1,base));building=false;
    // Unresolved or malformed collision facts never grant permission to shoot through an obstacle.
    resolvable=false;assert(!WeaponTargetFilter::PassThrough(1,base));resolvable=true;
    Radius(std::numeric_limits<float>::quiet_NaN());assert(!WeaponTargetFilter::PassThrough(1,base));
    Radius(-1);assert(!WeaponTargetFilter::PassThrough(1,base));
    assert(!UnitCollisionProfile::Read(1,base).known);
    Radius(std::numeric_limits<float>::quiet_NaN());assert(!UnitCollisionProfile::Read(1,base).known);
    Radius(99999);assert(!UnitCollisionProfile::Read(1,base).known);
    Radius(16);resolvable=false;assert(!UnitCollisionProfile::Read(1,base).known);resolvable=true;
    assert(!WeaponTargetFilter::PassThrough(0,base) && !WeaponTargetFilter::PassThrough(1,0));
    VirtualFree(memory,0,MEM_RELEASE);
}
