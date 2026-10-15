#include "NativeFramePresentation.hpp"
#include "WarcraftApi.hpp"
#include <gl/GL.h>
#include <intrin.h>
#include <cstring>

namespace {
using Flush=void (__thiscall*)(uintptr_t,int);
using Swap=BOOL (WINAPI*)(HDC,UINT);
Flush originalFlush=nullptr;
Swap originalSwap=nullptr;
NativeFramePresentation::Frame draw=nullptr;
uintptr_t completion=0;
thread_local bool submitting=false;

void Submit(HDC dc,const char* route) {
    // Resource/font work can invoke native callbacks; compose only once at the outer frame boundary.
    if(submitting || !dc || !wglGetCurrentContext())return;
    submitting=true;draw(dc,route);submitting=false;
}
void __fastcall FlushHook(uintptr_t renderer,uintptr_t,int finish) {
    uintptr_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    originalFlush(renderer,finish);
    // All world/UI batches are submitted. Capture calls outside the native present method
    // are not frame boundaries; presentation may use a different Windows/OpenGL entry point.
    if(completion && caller==completion)Submit(wglGetCurrentDC(),"native-frame");
}
BOOL WINAPI SwapHook(HDC dc,UINT planes) {
    if(planes&WGL_SWAP_MAIN_PLANE)Submit(dc,"legacy-swap");
    return originalSwap(dc,planes);
}
bool FrameBoundary(uintptr_t base) {
    const unsigned char flush[]={0x83,0xEC,0x0C,0x53,0x55,0x56,0x8B,0xF1};
    if(memcmp(reinterpret_cast<void*>(base+0x528FA0),flush,sizeof(flush)))return false;
    auto call=reinterpret_cast<const unsigned char*>(base+0x533294);
    int displacement=0;memcpy(&displacement,call+1,sizeof(displacement));
    // Validate caller and adapter; relocation does not change relative calls.
    return call[0]==0xE8 && base+0x533299+displacement==base+0x528FA0;
}
}
bool NativeFramePresentation::Install(uintptr_t gameBase,Hook hook,Frame frame) {
    draw=frame;completion=0;
    bool native=FrameBoundary(gameBase);
    if(native) {
        completion=gameBase+0x533299;
        native=hook(reinterpret_cast<void*>(gameBase+0x528FA0),reinterpret_cast<void*>(FlushHook),reinterpret_cast<void**>(&originalFlush));
        if(!native)completion=0;
    }
    // Unknown/modified layouts retain the previous API route rather than executing guessed code.
    bool legacy=false;
    if(!native) {
        HMODULE gl=LoadLibraryA("opengl32.dll");
        legacy=hook(reinterpret_cast<void*>(GetProcAddress(gl,"wglSwapLayerBuffers")),
            reinterpret_cast<void*>(SwapHook),reinterpret_cast<void**>(&originalSwap));
    }
    wc3::Log("Frame presentation installed native=%d legacySwap=%d",native,legacy);
    return native || legacy;
}
