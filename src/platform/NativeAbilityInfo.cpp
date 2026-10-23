#include "GameBuild.hpp"
#include "NativeAbilityInfo.hpp"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
template<class T> bool Load(uintptr_t at,T& value) {
    SIZE_T got=0;return at && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&value,sizeof(value),&got) && got==sizeof(value);
}
std::string Text(uintptr_t at) {
    std::string result;
    for(unsigned i=0;at && i<512;++i) {char c=0;if(!Load(at+i,c))return {};if(!c)return result;result+=c;}
    return {};
}
bool Timer(uintptr_t at) {
    // Check the actual RTTI before interpreting a polymorphic ability's optional timer field.
    uintptr_t table=0,locator=0,type=0;
    return Load(at,table) && Load(table-sizeof(uintptr_t),locator) && Load(locator+12,type) &&
        Text(type+8)==".?AVCAgentTimer@@";
}
}
std::string NativeAbilityInfo::ClassName(uintptr_t ability) {
    uintptr_t table=0,locator=0,type=0;
    // Same guarded MSVC RTTI path used for the optional native ability timer below.
    if(!Load(ability,table) || !Load(table-sizeof(uintptr_t),locator) || !Load(locator+12,type))return {};
    return Text(type+8);
}
bool NativeAbilityInfo::Definition(uintptr_t base,int ability,int& column,int& row,std::string& icon) {
    // Retain the native path until this internal adapter has verified 1.27a signatures.
    if(wc3::ActiveBuild()==wc3::GameBuildKind::Classic127a) {return false;}

    if(!base || !ability)return false;
    const unsigned char signature[]={0x51,0x53,0x8B,0xD9,0x57};
    if(std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x32C8E0)),signature,sizeof(signature)))return false;
    using UI=uintptr_t (__thiscall*)(int);
    uintptr_t ui=reinterpret_cast<UI>(base + wc3::Rva(0x32C8E0))(ability),items=0,art=0;unsigned count=0;
    if(!Load(ui+0x60,column) || !Load(ui+0x64,row) || column<-16 || column>16 || row<-16 || row>16)return false;
    // Native definitions include war3map.w3a and runtime changes even for skills not yet learned.
    if(Load(ui+0xB0,count) && count>0 && count<=100 && Load(ui+0xB4,items) && Load(items,art))icon=Text(art);
    return true;
}
void NativeAbilityInfo::Read(uintptr_t ability,SpellHudSlot& slot,float mana) {
    if(!ability)return;
    uintptr_t data=0,ui=0,items=0,art=0;unsigned levels=0;
    // The native per-level Art array includes custom object editor/runtime overrides.
    if(Load(ability+0x54,data) && Load(data+0x28,ui) && Load(ui+0xB0,levels) &&
        levels>0 && levels<=100 && Load(ui+0xB4,items) &&
        Load(items+sizeof(uintptr_t)*size_t(std::clamp(slot.level,1,int(levels))-1),art)) {
        auto icon=Text(art);if(!icon.empty())slot.icon=icon;
    }
    // Disabled buttons can lose their ability reference; retain the actor's native level costs as a fallback.
    unsigned count=0;uintptr_t definitions=0;
    if(slot.level>0 && Load(data+0x50,count) && count>0 && count<=100 && unsigned(slot.level)<=count && Load(data+0x54,definitions)) {
        uintptr_t current=definitions+size_t(slot.level-1)*0x68;int cost=0;float cooldown=0;
        if(Load(current+0x10,cost) && cost>=0 && cost<=1000000 && std::isfinite(mana) && mana>=0)slot.noMana=float(cost)>mana+.01f;
        if(Load(current+0x14,cooldown) && std::isfinite(cooldown) && cooldown>0 && cooldown<=86400)slot.cooldownTotal=cooldown;
    }
    uintptr_t running=0,clock=0;
    if(!Timer(ability+0xD0))return;
    // The native current-level cooldown supports Warcraft's radial progress, including runtime overrides.
    float total=0;int disabled=0;
    if(Load(ability+0xB4,total) && std::isfinite(total) && total>0 && total<=86400)slot.cooldownTotal=total;
    if(Load(ability+0x40,disabled) && disabled>=0 && disabled<100000)slot.disabled=disabled>0;
    slot.cooldownKnown=true;slot.cooldown=0;
    if(!Load(ability+0xDC,running)) {slot.cooldownKnown=false;return;}
    if(!running)return;
    float timeout=0,current=0;
    if(!Load(running+4,timeout) || !Load(running+0xC,clock) || !Load(clock+0x40,current) ||
        !std::isfinite(timeout) || !std::isfinite(current) || timeout-current>86400) {slot.cooldownKnown=false;return;}
    // Native time pauses with Warcraft and includes cooldown resets; wall-clock estimates would diverge.
    slot.cooldown=std::max(0.f,timeout-current);
}
