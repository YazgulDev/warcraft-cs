#include "NativeUnitHud.hpp"
#include <algorithm>
#include <array>
#include <cstring>

namespace {
template<class T> bool Load(uintptr_t at,T& value) {
    SIZE_T got=0;return at && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&value,sizeof(value),&got) && got==sizeof(value);
}
std::string Text(uintptr_t at) {
    std::string result;
    for(size_t i=0;at && i<512;++i) {char c=0;if(!Load(at+i,c))return {};if(!c)return result;result+=c;}
    return {};
}
}
void NativeUnitHud::Configure(uintptr_t base) {
    base_=base;
    std::array<unsigned char,8> code={};uintptr_t key=0;
    // Native level lookup clamps against this map's MaxHeroLevel and uses its already resolved XP table.
    const std::array<unsigned char,8> experience={0x56,0x8B,0x74,0x24,0x08,0x6A,0x00,0xBA};
    experienceVerified_=base && Load(base+0x207760,code) && code==experience &&
        Load(base+0x207768,key) && key==base+0x876208;
    const std::array<unsigned char,4> art={0x56,0x8B,0xF2,0xBA};std::array<unsigned char,4> artCode={};
    artVerified_=base && Load(base+0x32DCB0,artCode) && artCode==art &&
        Load(base+0x32DCB4,key) && key==base+0x938FE0;
    wc3::Log("Unit HUD native experience=%d itemArt=%d",experienceVerified_,artVerified_);
}
UnitHudSnapshot NativeUnitHud::Read(wc3::Handle actor,bool inventory) const {
    UnitHudSnapshot result;if(!actor || !wc3::GetUnitTypeId(actor))return result;
    result.actor=actor;result.health=wc3::Real(wc3::GetUnitState(actor,0));result.maxHealth=wc3::Real(wc3::GetUnitState(actor,1));
    result.mana=wc3::Real(wc3::GetUnitState(actor,2));result.maxMana=wc3::Real(wc3::GetUnitState(actor,3));
    result.hero=wc3::IsUnitType(actor,0)!=FALSE;
    if(result.hero) {
        result.level=wc3::GetHeroLevel(actor);result.experience=wc3::GetHeroXP(actor);
        if(experienceVerified_ && result.level>0 && result.level<100000) {
            using Threshold=int (__stdcall*)(int);
            auto threshold=reinterpret_cast<Threshold>(base_+0x207760);
            // This native table is indexed by the level being completed: entry 1 is XP for level 2.
            result.levelExperience=result.level>1 ? threshold(result.level-1) : 0;
            result.nextExperience=threshold(result.level);
            if(threshold(result.level+1)==result.nextExperience)result.nextExperience=result.levelExperience;
            result.experienceKnown=result.levelExperience>=0 && result.nextExperience>=result.levelExperience;
        }
    }
    if(inventory) {
        result.inventorySize=std::clamp(wc3::UnitInventorySize(actor),0,6);
        for(int i=0;i<result.inventorySize;++i) {
            auto item=wc3::UnitItemInSlot(actor,i);if(!item)continue;
            auto& slot=result.items[i];slot.type=wc3::GetItemTypeId(item);slot.charges=std::max(0,wc3::GetItemCharges(item));
            // Native buttonArt includes current-map object modifications and imported icons.
            if(artVerified_ && slot.type) {
                using Art=uintptr_t (__fastcall*)(int,int);
                slot.icon=Text(reinterpret_cast<Art>(base_+0x32DCB0)(slot.type,0));
            }
        }
    }
    return result;
}
