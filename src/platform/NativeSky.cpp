#include "GameBuild.hpp"
#include "NativeSky.hpp"
#include "WarcraftApi.hpp"
#include "../config/WarcraftSkyPresets.hpp"
#include <cstring>

bool NativeSky::Configure(uintptr_t base) {
    // SetSkyModel's verified 1.26a backend owns sprite loading and the native sky update callbacks.
    const unsigned char set[]={0x56,0x8B,0xF1,0x8B,0x8E,0x54,0x03,0x00,0x00};
    const unsigned char get[]={0x8B,0x41,0x08,0x85,0xC0,0x74,0x04,0x8B,0x40,0x1C,0xC3};
    if (memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x390590)),set,sizeof(set)) ||
        memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x4C4630)),get,sizeof(get))) return false;
    base_=base;return true;
}
void NativeSky::Reset() {
    // Unloaded map pointers must never be restored into the next map.
    frame_=0;lease_.Reset();
}
void NativeSky::Update(uintptr_t ui,bool enabled,char tileset,bool hidden,const char* preset) {
    if(!base_ || !ui)return;
    uintptr_t frame=*reinterpret_cast<uintptr_t*>(ui+0x3BC);if(!frame)return;
    if(frame_!=frame){Reset();frame_=frame;}
    using Set=void (__thiscall*)(uintptr_t,const char*);
    using Get=const char* (__fastcall*)(uintptr_t,uintptr_t);
    const char* current=reinterpret_cast<Get>(base_+wc3::Rva(0x4C4630))(frame+0x348,0);
    auto selected=WarcraftSkyPresets::All[WarcraftSkyPresets::Index(preset)].model;
    auto fallback=WarcraftSkyPresets::All[strchr("WNIC",tileset) && tileset ? 3 : 1].model;
    // A pure lease retains authored/script replacements across preset, visibility, CS and RTS transitions.
    auto change=lease_.Update(current ? current : "",*reinterpret_cast<uintptr_t*>(frame+0x354)!=0,enabled,hidden,selected,fallback);
    if(change){reinterpret_cast<Set>(base_+wc3::Rva(0x390590))(frame,change->empty() ? nullptr : change->c_str());
        wc3::Log("Native FPS sky model=%s",change->empty() ? "none" : change->c_str());}
}
