#include <cassert>
#include <cstdio>
// Exercise the real probe entry point without installing hooks or starting its background threads.
#include "../src/platform/D3DHooks.cpp"
namespace wc3 {void Log(const char*,...) {}}
// This probe oracle never submits native UI; that adapter has its own real-device pixel test.
namespace D3DNativeUi {
void Attach(IDirect3DDevice9*) {} void Finish() {} void Overlay(bool) {}
void BeforeReset(IDirect3DDevice9*) {} void Viewport(IDirect3DDevice9*,D3DVIEWPORT9&,int,int) {}
void Projection(IDirect3DDevice9*,bool) {}
}

int main() {
    char directory[MAX_PATH]={};GetSystemDirectoryA(directory,MAX_PATH);
    strcat_s(directory,"\\d3d9.dll");auto runtime=LoadLibraryA(directory);assert(runtime);
    HWND hidden=CreateWindowExA(0,"STATIC","D3D probe safety fixture",WS_POPUP,0,0,8,8,nullptr,nullptr,nullptr,nullptr);
    assert(hidden && !IsWindowVisible(hidden));
    D3DHooks::window=hidden;
    D3DHooks::sources[0].module=runtime;
    D3DHooks::sources[1].module=runtime;
    // A real D3D module is available, but an inactive game must not even allocate the helper window/device.
    assert(!D3DHooks::ProbeDevice(0));assert(!D3DHooks::probeWindow && !D3DHooks::probeDevice);
    D3DHooks::sources[0].frameEndpointsHooked=true;
    assert(D3DHooks::ProbeDevice(1));assert(!D3DHooks::probeWindow && !D3DHooks::probeDevice);
    // An unrelated runtime is not covered by an existing anchor just because another slot was hooked.
    D3DHooks::sources[1].module=GetModuleHandleA("kernel32.dll");
    assert(!D3DHooks::RuntimeAnchored(1));
    DestroyWindow(hidden);FreeLibrary(runtime);
    std::puts("PASS real D3D probe: hidden-window suppression, same-runtime alias reuse, distinct-runtime isolation, no helper allocation.");
}
