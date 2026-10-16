#include "NativeSpellSlots.hpp"
#include "NativeAbilityInfo.hpp"
#include "NativeAbilitySlots.hpp"
#include "NativeCommandCard.hpp"
#include "NativeArchive.hpp"
#include "MapSpellCharges.hpp"
#include <cstring>
#include <cctype>

namespace {
std::string ArchiveText(const char* path) {
    auto bytes=NativeArchive::Read(path,4*1024*1024);return std::string(bytes.begin(),bytes.end());
}
template<class T> bool Read(uintptr_t at,T& result) {
    SIZE_T got=0;return at && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&result,sizeof(result),&got) && got==sizeof(result);
}
}
void NativeSpellSlots::Configure(uintptr_t base) {
    base_=base;
    // Same lookup used by GetUnitAbilityLevel; fail closed on an unsupported native implementation.
    const unsigned char signature[]={0x8B,0x44,0x24,0x04,0x3D,0x75,0x62,0x4E,0x41};
    verified_=!memcmp(reinterpret_cast<void*>(base+0x787D0),signature,sizeof(signature));
    wc3::Log("Native spell ability lookup verified=%d",verified_);
}
std::array<int,4> NativeSpellSlots::Resolve(wc3::Handle unit) {
    if(!loaded_) {
        loaded_=true;slots_.LoadUnits(ArchiveText("Units\\UnitAbilities.slk"));
        for(const char* race:{"Human","Orc","Undead","NightElf","Neutral"}) {
            std::string path="Units\\"+std::string(race)+"AbilityFunc.txt";slots_.LoadPositions(ArchiveText(path.c_str()));
        }
    }
    int type=wc3::GetUnitTypeId(unit);
    if(verified_)for(int id:slots_.Declared(type)) {
        int column=0,row=0;std::string icon;
        if(NativeAbilityInfo::Definition(base_,id,column,row,icon))slots_.Presentation(id,column,row,icon);
    }
    auto resolved=slots_.ForUnit(type,[&](int ability){return wc3::GetUnitAbilityLevel(unit,ability)>0;});
    // DotA replaces research placeholders with other live rawcodes; casting and HUD share this current mapping.
    if(verified_) {
        auto live=NativeAbilitySlots::Read(base_,wc3::UnitObject(unit));
        for(int i=0;i<4;++i)if(live[i] && wc3::GetUnitAbilityLevel(unit,live[i])>0)resolved[i]=live[i];
    }
    return resolved;
}
int NativeSpellSlots::Ability(wc3::Handle unit,int slot,const SpellSettings& settings,bool diagnostics) {
    if(slot<0 || slot>=4)return 0;
    auto resolved=Resolve(unit);
    // Explicit player rawcodes take precedence over the shared live mapping.
    if(!settings.ability[slot].empty())return HeroSpellSlots::Rawcode(settings.ability[slot]);
    if(diagnostics)wc3::Log("Hero spell slots unit=%08X type=%08X resolved=%08X,%08X,%08X,%08X requestedSlot=%d",
        unit,wc3::GetUnitTypeId(unit),resolved[0],resolved[1],resolved[2],resolved[3],slot+1);
    return resolved[slot];
}
int NativeSpellSlots::Order(wc3::Handle unit,int ability) {
    if(!verified_ || !ability || slots_.Passive(ability) || wc3::GetUnitAbilityLevel(unit,ability)<=0)return 0;
    uintptr_t skill=Object(unit,ability),table=0,method=0;
    if(!Read(skill,table) || !Read(table+0x308,method))return 0;
    if(method<base_+0x1000 || method>=base_+0x880000)return 0;
    // Stock spell order getters are mov eax,<order>; ret. Read the result without invoking aura vtables.
    std::array<unsigned char,6> code={};if(!Read(method,code) || code[0]!=0xB8 || code[5]!=0xC3)return 0;
    int order=0;std::memcpy(&order,code.data()+1,4);
    return order>=852000 && order<=853999 ? order : 0;
}
uintptr_t NativeSpellSlots::Object(wc3::Handle unit,int ability) const {
    if(!verified_ || !ability)return 0;
    using Find=uintptr_t (__thiscall*)(uintptr_t,int,BOOL,BOOL,BOOL,BOOL);
    uintptr_t object=wc3::UnitObject(unit);if(!object)return 0;
    uintptr_t skill=reinterpret_cast<Find>(base_+0x787D0)(object,ability,FALSE,TRUE,TRUE,TRUE),table=0;
    return Read(skill,table) && table>=base_+0x880000 && table<base_+0xA80000 ? skill : 0;
}
bool NativeSpellSlots::PointOnly(wc3::Handle unit,int ability,int order) {
    if(!verified_)return false;
    using GetUI=uintptr_t (__fastcall*)(int,int);
    uintptr_t ui=reinterpret_cast<GetUI>(base_+0x300710)(0,0);
    return NativeCommandCard::PointOnly(base_,ui,Object(unit,ability),ability,order);
}
SpellHudSlots NativeSpellSlots::Snapshot(wc3::Handle unit,const SpellSettings& settings) {
    SpellHudSlots result={};if(!unit)return result;
    auto resolved=Resolve(unit);
    // Sample all learned/passive gaps using the very same mapping consumed by spell casting.
    using GetUI=uintptr_t (__fastcall*)(int,int);
    uintptr_t ui=verified_ ? reinterpret_cast<GetUI>(base_+0x300710)(0,0) : 0;
    float mana=wc3::Real(wc3::GetUnitState(unit,2));
    for(int i=0;i<4;++i) {
        auto& slot=result[i];slot.ability=settings.ability[i].empty() ? resolved[i] : HeroSpellSlots::Rawcode(settings.ability[i]);
        if(!slot.ability)continue;
        slot.level=wc3::GetUnitAbilityLevel(unit,slot.ability);slot.passive=slots_.Passive(slot.ability);
        uintptr_t object=Object(unit,slot.ability);
        slot.icon=slots_.Icon(slot.ability);NativeAbilityInfo::Read(object,slot,mana);
        NativeCommandCard::Apply(ui,object,mana,slot);
        MapSpellCharges::Read(unit,slot);
        // Runtime replacement abilities can be passive without appearing in the hero's original tables.
        auto lower=slot.icon;for(auto& c:lower)c=char(std::tolower(static_cast<unsigned char>(c)));
        if(lower.find("passivebuttons\\")!=std::string::npos)slot.passive=true;
    }
    return result;
}
