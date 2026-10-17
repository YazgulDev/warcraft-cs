#include "NativeItemCommands.hpp"
#include <array>
#include <utility>

namespace {
template<class T> bool Load(uintptr_t at,T& value) {
    SIZE_T got=0;return at && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&value,sizeof(value),&got) && got==sizeof(value);
}
}
unsigned NativeItemCommands::Forms(uintptr_t base,wc3::Handle actor,int slot,wc3::Handle item) {
    if(!base || slot<0 || slot>=6 || !item || wc3::UnitItemInSlot(actor,slot)!=item)return 0;
    // These native CItem getters perform the engine's live active-ability/type checks, including map overrides.
    std::array<unsigned char,3> expected={0x6A,0xFF,0x68},code={};uintptr_t handler=0;
    for(auto entry : {std::pair<uintptr_t,uintptr_t>{0x3BEB50,0x825578},{0x2B6CC0,0x817098},{0x2B6E20,0x8170C8}}) {
        if(!Load(base+entry.first,code) || code!=expected || !Load(base+entry.first+3,handler) || handler!=base+entry.second)return 0;
    }
    using Item=uintptr_t (__fastcall*)(wc3::Handle,int);
    using Order=int (__thiscall*)(uintptr_t,int);
    using Flags=unsigned (__thiscall*)(uintptr_t);
    auto object=reinterpret_cast<Item>(base+0x3BEB50)(item,0);if(!object)return 0;
    int command=reinterpret_cast<Order>(base+0x2B6CC0)(object,0);
    if(command!=852008+slot)return 0; // Passive items have no activation order.
    unsigned forms=reinterpret_cast<Flags>(base+0x2B6E20)(object)&7;
    wc3::Log("Inventory native slot=%d order=%d forms=%u",slot+1,command,forms);return forms;
}
