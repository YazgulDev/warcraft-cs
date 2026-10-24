#include "GameBuild.hpp"
#include "NativeAbilitySlots.hpp"
#include <windows.h>
#include <unordered_set>

namespace {
template<class T> bool Load(uintptr_t at,T& value) {
    SIZE_T got=0;return at && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&value,sizeof(value),&got) && got==sizeof(value);
}
uintptr_t Next(uintptr_t base,uintptr_t link) {
    std::array<uint32_t,2> key={};if(!Load(link,key) || (key[0]&key[1])==0xFFFFFFFF)return 0;
    // Warcraft's generation resolver rejects deleted/reused nodes, as in GetUnitAbilityLevel.
    using Resolve=uintptr_t (__thiscall*)(uintptr_t);
    return reinterpret_cast<Resolve>(base + wc3::Rva(0x4786B0))(link);
}
}
std::vector<int> NativeAbilitySlots::All(uintptr_t base,uintptr_t unit) {
    std::vector<int> result;if(!base || !unit)return result;
    uintptr_t ability=Next(base,unit+0x1DC);std::unordered_set<uintptr_t> seen;
    for(int i=0;ability && i<128 && seen.insert(ability).second;++i) {
        uintptr_t table=0;int id=0;
        // Keep only live native ability instances; casting separately rejects passives/unlearned helpers.
        if(Load(ability,table) && table>=base + 0x880000 && table<base + 0xA80000 && Load(ability+0x34,id) && id)
            result.push_back(id);
        ability=Next(base,ability+0x24);
    }
    return result;
}
std::array<int,4> NativeAbilitySlots::Read(uintptr_t base,uintptr_t unit) {
    // Retain the native path until this internal adapter has verified 1.27a signatures.
    if(wc3::ActiveBuild()==wc3::GameBuildKind::Classic127a) {return {};}

    std::array<int,4> result={};if(!base || !unit)return result;
    uintptr_t ability=Next(base,unit+0x1DC);std::unordered_set<uintptr_t> seen;
    for(int i=0;ability && i<128 && seen.insert(ability).second;++i) {
        uintptr_t table=0,data=0,ui=0;int id=0,x=-1,y=-1,researchY=-1;
        // Normal hero spells occupy the bottom command row. Hidden helpers and the stats row do not.
        if(Load(ability,table) && table>=base + 0x880000 && table<base + 0xA80000 &&
            Load(ability+0x34,id) && Load(ability+0x54,data) && Load(data+0x28,ui) &&
            Load(ui+0x50,x) && Load(ui+0x54,y) && Load(ui+0x64,researchY) &&
            x>=0 && x<4 && y==2 && researchY==0)result[size_t(x)]=id;
        ability=Next(base,ability+0x24);
    }
    return result;
}
