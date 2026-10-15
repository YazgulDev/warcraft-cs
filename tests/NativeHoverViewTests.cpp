#include "../src/platform/NativeHoverView.hpp"
#include "../src/platform/WarcraftApi.hpp"
#include <array>
#include <vector>
#include <cassert>
#include <cstring>
#include <cstdio>

namespace wc3 {void Log(const char*,...) {}}
namespace {
using Setter=void (__fastcall*)(uintptr_t,uintptr_t,BOOL,BOOL);
Setter detour=nullptr;
int calls=0;
void __fastcall Apply(uintptr_t ui,uintptr_t,BOOL input,BOOL presentation) {
    ++calls;*reinterpret_cast<BOOL*>(ui+0x298)=input;*reinterpret_cast<BOOL*>(ui+0x29C)=presentation;
}
bool Capture(void*,void* replacement,void** original) {
    detour=reinterpret_cast<Setter>(replacement);*original=reinterpret_cast<void*>(Apply);return true;
}
BOOL& Input(uintptr_t ui){return *reinterpret_cast<BOOL*>(ui+0x298);}
BOOL& Visual(uintptr_t ui){return *reinterpret_cast<BOOL*>(ui+0x29C);}
}
int main() {
    // The actual native adapter refuses an unknown host before touching UI state.
    std::vector<unsigned char> image(0x2F7000,0);
    uintptr_t base=reinterpret_cast<uintptr_t>(image.data());
    assert(!NativeHoverView::Install(base,Capture) && !detour);
    const unsigned char signature[]={0x8B,0x44,0x24,0x04,0x89,0x81,0x98,0x02,0x00,0x00,
        0x8B,0x44,0x24,0x08,0x85,0xC0,0x89,0x81,0x9C,0x02,0x00,0x00};
    std::memcpy(image.data()+0x2F6F90,signature,sizeof(signature));assert(NativeHoverView::Install(base,Capture));
    alignas(4) std::array<unsigned char,0x300> first={},second={};
    auto ui=reinterpret_cast<uintptr_t>(first.data()),other=reinterpret_cast<uintptr_t>(second.data());
    Input(ui)=Visual(ui)=TRUE;
    NativeHoverView::Update(ui,false);assert(calls==0);
    NativeHoverView::Update(ui,true);assert(!Input(ui) && !Visual(ui) && calls==1);
    NativeHoverView::Update(ui,true);assert(calls==1);
    // A script changes its preference during F9: suppress now, restore the latest pair on exit/F8 opt-out.
    detour(ui,0,FALSE,TRUE);assert(!Input(ui) && !Visual(ui));
    detour(other,0,TRUE,FALSE);assert(Input(other) && !Visual(other));
    NativeHoverView::Update(ui,false);assert(!Input(ui) && Visual(ui));
    detour(ui,0,TRUE,FALSE);NativeHoverView::Update(ui,true);NativeHoverView::Update(ui,false);
    assert(Input(ui) && !Visual(ui));
    // Map replacement never writes the old UI. A new map captures and restores its own exact values.
    NativeHoverView::Update(ui,true);NativeHoverView::Reset();Input(ui)=7;Visual(ui)=8;
    Input(other)=2;Visual(other)=3;NativeHoverView::Update(other,true);
    NativeHoverView::Update(other,false);assert(Input(other)==2 && Visual(other)==3 && Input(ui)==7 && Visual(ui)==8);
    NativeHoverView::Update(other,true);NativeHoverView::Update(0,false);
    assert(!Input(other) && !Visual(other)); // Unloaded UI storage is never revisited.
    std::puts("Native hover signature, F9 borrowing, script preferences, opt-out and map replacement passed.");
}
