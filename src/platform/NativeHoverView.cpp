#include "NativeHoverView.hpp"
#include "WarcraftApi.hpp"
#include <cstring>

namespace {
using Setter=void (__thiscall*)(uintptr_t,BOOL,BOOL);
Setter original=nullptr;
uintptr_t owner=0;
BOOL savedInput=FALSE,savedUI=FALSE;
bool applied=false;
void __fastcall SetHook(uintptr_t ui,uintptr_t,BOOL input,BOOL presentation) {
    // Keep script/cinematic requests as the new restoration state, without flashing a circle in F9.
    if(applied && ui==owner) {
        savedInput=input;savedUI=presentation;original(ui,FALSE,FALSE);
    } else original(ui,input,presentation);
}
}
bool NativeHoverView::Install(uintptr_t base,HookInstaller hook) {
    // 1.26a EnablePreSelect delegates here: two BOOLs at UI+298/+29C, callee pops eight bytes.
    const unsigned char signature[]={0x8B,0x44,0x24,0x04,0x89,0x81,0x98,0x02,0x00,0x00,
        0x8B,0x44,0x24,0x08,0x85,0xC0,0x89,0x81,0x9C,0x02,0x00,0x00};
    void* target=reinterpret_cast<void*>(base+0x2F6F90);
    if(std::memcmp(target,signature,sizeof(signature))) {
        wc3::Log("Native hover signature mismatch; preselection retained");return false;
    }
    return hook(target,reinterpret_cast<void*>(SetHook),reinterpret_cast<void**>(&original));
}
void NativeHoverView::Update(uintptr_t ui,bool hide) {
    if(!original)return;
    if(owner!=ui) {applied=false;owner=ui;} // A different map has independent native preferences.
    if(!ui)return;
    if(hide && !applied) {
        savedInput=*reinterpret_cast<BOOL*>(ui+0x298);savedUI=*reinterpret_cast<BOOL*>(ui+0x29C);
        applied=true;
        // The native setter also removes the already displayed hover circle and refreshes widget highlights.
        original(ui,FALSE,FALSE);wc3::Log("Unit-view hover hidden; saved input=%d ui=%d",savedInput,savedUI);
    } else if(!hide && applied) {
        applied=false;original(ui,savedInput,savedUI);
        wc3::Log("Unit-view hover restored input=%d ui=%d",savedInput,savedUI);
    }
}
void NativeHoverView::Reset() {owner=0;applied=false;}
