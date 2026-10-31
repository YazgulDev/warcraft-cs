#include "GameBuild.hpp"
#include "NativeSky.hpp"
#include "WarcraftApi.hpp"
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
    frame_=0;applied_=hidden_=false;model_.clear();hiddenMapModel_.clear();
}
void NativeSky::Update(uintptr_t ui, bool enabled, char tileset,bool hidden) {
    if (!base_ || !ui) return;
    uintptr_t frame=*reinterpret_cast<uintptr_t*>(ui+0x3BC);
    if (!frame) return;
    if (frame_!=frame) {Reset();frame_=frame;}
    using Set=void (__thiscall*)(uintptr_t,const char*);
    using Get=const char* (__fastcall*)(uintptr_t,uintptr_t);
    auto set=reinterpret_cast<Set>(base_ + wc3::Rva(0x390590));
    const char* current=reinterpret_cast<Get>(base_ + wc3::Rva(0x4C4630))(frame+0x348,0);
    // Map scripts may replace our preview during a cinematic; never undo their new sky.
    if (applied_ && (!current || _stricmp(current,model_.c_str()))) applied_=false;
    // Unlike the old fill-missing-sky preference, visibility also hides a map-authored sky.
    // Preserve the newest map replacement and restore only while our cleared slot is still empty.
    if(hidden) {
        if(!hidden_ || (current && *current)) {
            hiddenMapModel_=applied_ ? "" : current ? current : "";
            set(frame,nullptr);applied_=false;hidden_=true;wc3::Log("Native FPS sky hidden");
        }
        return;
    }
    if(hidden_) {
        if((!current || !*current) && !hiddenMapModel_.empty())set(frame,hiddenMapModel_.c_str());
        hidden_=false;hiddenMapModel_.clear();current=reinterpret_cast<Get>(base_+wc3::Rva(0x4C4630))(frame+0x348,0);
        wc3::Log("Native FPS sky visibility restored");
    }
    if (!enabled) {
        if (applied_) {set(frame,nullptr);wc3::Log("Native FPS sky restored to map default");}
        applied_=false;return;
    }
    // Existing map skies stay intact. Only the otherwise empty RTS sky gains a native FPS backdrop.
    if (!applied_ && (!current || !*current) && !*reinterpret_cast<uintptr_t*>(frame+0x354)) {
        model_=strchr("WNIC",tileset) && tileset ?
            "Environment\\Sky\\LordaeronWinterSky\\LordaeronWinterSky.mdl" :
            "Environment\\Sky\\LordaeronSummerSky\\LordaeronSummerSky.mdl";
        set(frame,model_.c_str());applied_=true;
        wc3::Log("Native FPS sky enabled: %s",model_.c_str());
    }
}
