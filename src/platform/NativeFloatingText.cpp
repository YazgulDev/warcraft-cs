#include "NativeFloatingText.hpp"
#include "WarcraftApi.hpp"
#include "../presentation/WorldLabelVisibility.hpp"
#include <cstring>

namespace {
WorldLabelView view;
using Draw=void (__thiscall*)(uintptr_t, uintptr_t);
Draw originalDraw=nullptr;
void __fastcall DrawHook(uintptr_t manager, uintptr_t, uintptr_t tag) {
    // Bit 0 marks world coordinates; screen-space map/HUD labels keep their native path.
    if (view.active && tag && (*reinterpret_cast<unsigned char*>(tag+0x30)&1)) {
        float position[3];std::memcpy(position,reinterpret_cast<void*>(tag),sizeof(position));
        // Native velocity moves glyphs in screen space; culling uses their unchanged world anchor.
        bool visible=WorldLabelVisibility::Visible(view,position);
#ifdef WCS_FLOATING_TEXT_TEST
        static uintptr_t observed[12]={};static unsigned count=0;
        bool seen=false;for (unsigned i=0;i<count;++i) seen|=observed[i]==tag;
        if (!seen && count<12) {
            observed[count++]=tag;
            wc3::Log("Native world label visible=%d x=%.1f y=%.1f z=%.1f eye=%.1f,%.1f,%.1f",visible,position[0],position[1],position[2],view.eye[0],view.eye[1],view.eye[2]);
        }
#endif
        if (!visible)return;
    }
    originalDraw(manager,tag);
}
}
bool NativeFloatingText::Install(uintptr_t base, HookInstaller hook) {
    // Verified 1.26a CTextTagManager::DrawTag prologue; refuse different native layouts.
    const unsigned char signature[]={0x83,0xEC,0x2C,0x56,0x8B,0x74,0x24,0x34,0x83,0x7E,0x2C,0x00};
    void* target=reinterpret_cast<void*>(base+0x4E54B0);
    if (std::memcmp(target,signature,sizeof(signature))) {
        wc3::Log("Floating text signature mismatch; native renderer retained");return false;
    }
    return hook(target,reinterpret_cast<void*>(DrawHook),reinterpret_cast<void**>(&originalDraw));
}
void NativeFloatingText::SetView(const WorldLabelView& snapshot) { view=snapshot; }
