#include "../src/platform/NativeFramePresentation.hpp"
#include <gl/GL.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
namespace wc3 {void Log(const char*,...) {} void LogError(const char*,...) {}}
namespace {
using Flush=void (__fastcall*)(uintptr_t,uintptr_t,int);
using Swap=BOOL (WINAPI*)(HDC,UINT);
Flush flushHook=nullptr;Swap swapHook=nullptr;
uintptr_t target=0;int frames=0,flushes=0,swaps=0;const char* route=nullptr;
// A free x86 adapter uses fastcall's unused EDX to reproduce the native thiscall stack contract.
void __fastcall Original(uintptr_t renderer,uintptr_t,int finish) {assert(renderer==0x1234 && finish==17);++flushes;}
BOOL WINAPI SwapOriginal(HDC,UINT) {++swaps;return TRUE;}
bool Hook(void* address,void* callback,void** original) {
    if(reinterpret_cast<uintptr_t>(address)==target) {flushHook=reinterpret_cast<Flush>(callback);*original=reinterpret_cast<void*>(Original);return true;}
    if(address==reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA("opengl32.dll"),"wglSwapLayerBuffers"))) {
        swapHook=reinterpret_cast<Swap>(callback);*original=reinterpret_cast<void*>(SwapOriginal);return true;
    }
    return false;
}
void Draw(HDC dc,const char* name) {
    ++frames;route=name;assert(dc==wglGetCurrentDC());assert(flushes>0);
    glClearColor(0,1,0,1);glClear(GL_COLOR_BUFFER_BIT);
    flushHook(0x1234,0,17); // Resource callbacks inside composition must not produce another HUD.
}
void Signature(unsigned char* image) {
    const unsigned char bytes[]={0x83,0xEC,0x0C,0x53,0x55,0x56,0x8B,0xF1};memcpy(image+0x528FA0,bytes,sizeof(bytes));
    image[0x533294]=0xE8;*reinterpret_cast<int*>(image+0x533295)=0x528FA0-0x533299;
}
void Complete(unsigned char* image) {
    // Synthetic x86 caller invokes the real adapter after a native-shaped flush, with no game code/data.
    auto start=image+0x1000;start[0]=0xB9;*reinterpret_cast<unsigned*>(start+1)=0x1234;
    start[5]=0x31;start[6]=0xD2;start[7]=0x6A;start[8]=17;start[9]=0xE9;
    *reinterpret_cast<int*>(start+10)=int((image+0x533294)-(start+14));
    *reinterpret_cast<int*>(image+0x533295)=int(reinterpret_cast<unsigned char*>(flushHook)-(image+0x533299));
    image[0x533299]=0xC3;FlushInstructionCache(GetCurrentProcess(),image,0x534000);reinterpret_cast<void (__cdecl*)()>(start)();
}
}
int main() {
    HWND window=CreateWindowA("STATIC","Frame completion regression",WS_POPUP,0,0,32,32,nullptr,nullptr,nullptr,nullptr);assert(window);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));HGLRC context=wglCreateContext(dc);assert(wglMakeCurrent(dc,context));
    auto image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x534000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(image);target=reinterpret_cast<uintptr_t>(image+0x528FA0);Signature(image);
    assert(NativeFramePresentation::Install(reinterpret_cast<uintptr_t>(image),Hook,Draw));assert(flushHook && !swapHook);
    flushHook(0x1234,0,17);assert(frames==0 && flushes==1); // An upload/capture is not a frame.
    for(GLenum buffer : {GL_BACK,GL_FRONT}) {
        glDrawBuffer(buffer);Complete(image);assert(!strcmp(route,"native-frame"));
        glReadBuffer(buffer);unsigned char pixel[3]={};glReadPixels(0,0,1,1,GL_RGB,GL_UNSIGNED_BYTE,pixel);assert(pixel[1]==255);
    }
    assert(frames==2 && flushes==5);
    // Modified signatures keep the previous swap route without an unverified native detour.
    Signature(image);image[0x528FA0]=0x90;
    assert(NativeFramePresentation::Install(reinterpret_cast<uintptr_t>(image),Hook,Draw));assert(swapHook);
    swapHook(dc,WGL_SWAP_MAIN_PLANE);assert(frames==3 && swaps==1 && !strcmp(route,"legacy-swap"));
    swapHook(dc,0);assert(frames==3 && swaps==2);
    wglMakeCurrent(nullptr,nullptr);swapHook(dc,WGL_SWAP_MAIN_PLANE);assert(frames==3);
    wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);VirtualFree(image,0,MEM_RELEASE);
    puts("PASS native pre-presentation boundary, real HUD pixels, non-frame exclusion, nested calls and signature fallback.");
}
