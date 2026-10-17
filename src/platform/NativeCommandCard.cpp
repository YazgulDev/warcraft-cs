#include "NativeCommandCard.hpp"
#include <windows.h>
#include <cmath>
#include <array>
#include <cstring>

namespace {
template<class T> bool Load(uintptr_t at,T& value) {
    SIZE_T got=0;return at && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&value,sizeof(value),&got) && got==sizeof(value);
}
std::string Icon(uintptr_t at) {
    std::string result;
    for(size_t i=0;i<260;++i) {char c=0;if(!Load(at+i,c))return {};if(!c)return result;result+=c;}
    return {};
}
uintptr_t Button(uintptr_t ui,uintptr_t abilityObject,int ability) {
    if(!ui || !abilityObject || !ability)return 0;
    uintptr_t bar=0,items=0,rows=0,buttons=0;
    if(!Load(ui+0x3C8,bar) || !Load(bar+0x154,items) || !Load(items+8,rows) || !Load(rows,buttons))return 0;
    for(int i=0;i<12;++i) {
        uintptr_t data=0,object=0;int id=0;
        // A selected ally, research submenu or stale command card must not overwrite our hero's slots.
        if(!Load(buttons+i*0x1C0+0x190,data) || !Load(data+0x6D4,object) || object!=abilityObject ||
            !Load(data+4,id) || id!=ability)continue;
        return data;
    }
    return 0;
}
}
bool NativeCommandCard::Apply(uintptr_t ui,uintptr_t abilityObject,float mana,SpellHudSlot& slot) {
    uintptr_t data=Button(ui,abilityObject,slot.ability);if(!data)return false;
    auto path=Icon(data+0x5C8);if(!path.empty())slot.icon=path;
    int cost=0;
    if(Load(data+0x594,cost) && cost>=0 && cost<=1000000 && std::isfinite(mana))slot.noMana=float(cost)>mana+.01f;
    slot.commandKnown=true;return true;
}
bool NativeCommandCard::PointOnly(uintptr_t base,uintptr_t ui,uintptr_t abilityObject,int ability,int order) {
    // Consult the same order flags used by Warcraft's point/widget dispatch, only for this actor's button.
    if(!base || !order || !Button(ui,abilityObject,ability))return false;
    const std::array<unsigned char,7> expected={0x8B,0x44,0x24,0x04,0x56,0x8B,0xF1};
    std::array<unsigned char,7> signature={};uintptr_t bar=0;
    if(!Load(base+0x372C30,signature) || signature!=expected || !Load(ui+0x3C8,bar) || !bar)return false;
    using Flags=unsigned (__thiscall*)(uintptr_t,int);
    unsigned flags=reinterpret_cast<Flags>(base+0x372C30)(bar,order)&0x3FF;
    return (flags&2)!=0 && (flags&4)==0;
}
