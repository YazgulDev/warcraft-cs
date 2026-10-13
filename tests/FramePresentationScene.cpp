#include "FramePresentationScene.hpp"
#include "../src/platform/WarcraftApi.hpp"
#include <windows.h>
#include <gl/GL.h>

namespace {
uintptr_t* swapImport=nullptr;
uintptr_t previous=0;
BOOL WINAPI AlternateSwap(HDC dc,UINT planes) {
    // The same double-buffered frame uses GDI, bypassing the old OpenGL API hook.
    return (planes&WGL_SWAP_MAIN_PLANE) ? SwapBuffers(dc) : TRUE;
}
}
void FramePresentationScene::Install(uintptr_t base,Hook) {
    swapImport=reinterpret_cast<uintptr_t*>(base+0x86D44C);previous=*swapImport;
}
void FramePresentationScene::Alternate(bool enabled) {
    // Only the disposable oracle changes an import; ordinary modules never alter Warcraft's imports.
    DWORD protection=0;if(!swapImport || !VirtualProtect(swapImport,sizeof(*swapImport),PAGE_READWRITE,&protection))return;
    *swapImport=enabled ? reinterpret_cast<uintptr_t>(AlternateSwap) : previous;
    DWORD ignored=0;VirtualProtect(swapImport,sizeof(*swapImport),protection,&ignored);
    wc3::Log("fixture presentation alternate=%d",enabled);
}
