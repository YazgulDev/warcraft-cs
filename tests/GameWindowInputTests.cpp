#include "../src/platform/GameWindowInput.hpp"
#include <cassert>
#include <cstdio>

namespace wc3 { void Log(const char*,...) {} void LogError(const char*,...) {} }
static int nativeA=0,nativeB=0,modKeys=0,externalCalls=0;
static WNDPROC externalPrevious=nullptr;
static LRESULT CALLBACK ModProc(HWND,UINT,WPARAM,LPARAM);
static GameWindowInput input(ModProc);
static LRESULT CALLBACK NativeA(HWND w,UINT m,WPARAM k,LPARAM d) {
    if (m==WM_KEYDOWN && k==VK_F6) {++nativeA;return 11;}
    return DefWindowProcA(w,m,k,d);
}
static LRESULT CALLBACK NativeB(HWND w,UINT m,WPARAM k,LPARAM d) {
    if (m==WM_KEYDOWN && k==VK_F6) {++nativeB;return 22;}
    return DefWindowProcA(w,m,k,d);
}
static LRESULT CALLBACK ModProc(HWND w,UINT m,WPARAM k,LPARAM d) {
    auto previous=input.Previous(w);
    if (w==input.Window() && m==WM_KEYDOWN && k==VK_F6) {++modKeys;return 33;}
    auto result=previous ? CallWindowProcA(previous,w,m,k,d) : DefWindowProcA(w,m,k,d);
    // Match the runtime: forward final destruction before discarding the native procedure.
    if (m==WM_NCDESTROY) input.Destroyed(w);
    return result;
}
static LRESULT CALLBACK ExternalProc(HWND w,UINT m,WPARAM k,LPARAM d) {
    ++externalCalls;return CallWindowProcA(externalPrevious,w,m,k,d);
}
static HWND MakeWindow(const char* name,WNDPROC proc) {
    WNDCLASSA type={};type.lpfnWndProc=proc;type.hInstance=GetModuleHandleA(nullptr);type.lpszClassName=name;
    assert(RegisterClassA(&type));
    auto w=CreateWindowA(name,name,WS_OVERLAPPED,0,0,100,100,nullptr,nullptr,type.hInstance,nullptr);
    assert(w);return w;
}
int main() {
    auto a=MakeWindow("MovieInputA",NativeA);
    assert(input.Observe(a));assert(input.Previous(a)==NativeA);
    assert(SendMessageA(a,WM_KEYDOWN,VK_F6,0)==33 && modKeys==1 && nativeA==0);
    assert(!input.Observe(a));
    // A native movie player can restore the original procedure on the same HWND.
    SetWindowLongPtrA(a,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(NativeA));
    assert(SendMessageA(a,WM_KEYDOWN,VK_F6,0)==11 && nativeA==1);
    assert(input.Observe(a));assert(!input.Observe(a));
    assert(SendMessageA(a,WM_KEYDOWN,VK_F6,0)==33 && modKeys==2 && nativeA==1);
    // Preserve another subclass that already forwards into us; rewrapping would recurse.
    externalPrevious=reinterpret_cast<WNDPROC>(SetWindowLongPtrA(a,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(ExternalProc)));
    assert(externalPrevious==ModProc && !input.Observe(a));
    assert(SendMessageA(a,WM_KEYDOWN,VK_F6,0)==33 && modKeys==3 && externalCalls==1);
    auto b=MakeWindow("MovieInputB",NativeB);
    assert(input.Observe(b));assert(input.Previous(b)==NativeB);
    assert(SendMessageA(a,WM_KEYDOWN,VK_F6,0)==11 && nativeA==2 && nativeB==0);
    assert(SendMessageA(b,WM_KEYDOWN,VK_F6,0)==33 && modKeys==4);
    DestroyWindow(a);assert(input.Window()==b && !input.Previous(a));
    assert(!input.Observe(nullptr));assert(!input.Observe(GetDesktopWindow()));assert(!input.Observe(a));
    DestroyWindow(b);assert(!input.Window() && !input.Previous(b));
    auto c=MakeWindow("MovieInputC",NativeA);
    assert(input.Observe(c));assert(SendMessageA(c,WM_KEYDOWN,VK_F6,0)==33 && modKeys==5);
    DestroyWindow(c);
    puts("Game window input: native movie reset, replacement windows and subclass forwarding passed.");
}
