#include "MapCameraGuard.hpp"
#include "WarcraftApi.hpp"
#include <cstring>

namespace {
using Field = void (__cdecl*)(int, float*, float*);
using Follow = void (__cdecl*)(wc3::Handle, float*, float*, BOOL);
using Orient = void (__cdecl*)(wc3::Handle, float*, float*);
using Position = void (__cdecl*)(float*, float*);
using Setup = void (__cdecl*)(wc3::Handle, BOOL, BOOL);
using SetupTimed = void (__cdecl*)(wc3::Handle, BOOL, float*);
using SetupZ = void (__cdecl*)(wc3::Handle, float*);
using SetupTimedZ = void (__cdecl*)(wc3::Handle, float*, float*);
using PanTimed = void (__cdecl*)(float*, float*, float*);
using PanTimedZ = void (__cdecl*)(float*, float*, float*, float*);
using Reset = void (__cdecl*)(float*);
uintptr_t gameBase = 0;
bool cameraLayoutVerified = false;
Field field = nullptr;
Follow follow = nullptr;
Orient orient = nullptr;
Position position = nullptr;
Setup setup = nullptr;
SetupTimed setupTimed = nullptr;
SetupZ setupZ = nullptr;
SetupTimedZ setupTimedZ = nullptr;
PanTimed panTimed = nullptr;
PanTimedZ panTimedZ = nullptr;
Reset reset = nullptr;
bool (*blocked)() = nullptr;
unsigned blockedWrites = 0;
bool Block() {
    if (!blocked || !blocked()) return false;
    // Sparse diagnostics prove periodic RTS timers were intercepted without logging every frame.
    if (blockedWrites++ < 4) wc3::Log("FPS camera: blocked map RTS camera write");
    return true;
}
// Only live FPS intercepts map camera requests. Paused menus, RTS and yielded cinematics call the originals.
void __cdecl FieldHook(int f, float* v, float* t) { if (!Block()) field(f,v,t); }
void __cdecl FollowHook(wc3::Handle u, float* x, float* y, BOOL inherit) { if (!Block()) follow(u,x,y,inherit); }
void __cdecl OrientHook(wc3::Handle u, float* x, float* y) { if (!Block()) orient(u,x,y); }
void __cdecl PositionHook(float* x, float* y) { if (!Block()) position(x,y); }
void __cdecl SetupHook(wc3::Handle s, BOOL pan, BOOL duration) { if (!Block()) setup(s,pan,duration); }
void __cdecl SetupTimedHook(wc3::Handle s, BOOL pan, float* t) { if (!Block()) setupTimed(s,pan,t); }
void __cdecl SetupZHook(wc3::Handle s, float* z) { if (!Block()) setupZ(s,z); }
void __cdecl SetupTimedZHook(wc3::Handle s, float* z, float* t) { if (!Block()) setupTimedZ(s,z,t); }
void __cdecl PanTimedHook(float* x, float* y, float* t) { if (!Block()) panTimed(x,y,t); }
void __cdecl PanTimedZHook(float* x, float* y, float* z, float* t) { if (!Block()) panTimedZ(x,y,z,t); }
void __cdecl ResetHook(float* t) { if (!Block()) reset(t); }
}

void MapCameraGuard::PositionEyeTarget(float* x, float* y) {
    if (!cameraLayoutVerified) { wc3::SetCameraPosition(x,y); return; }
    using GetUI = uintptr_t (__fastcall*)(int,int);
    uintptr_t ui = reinterpret_cast<GetUI>(gameBase + 0x300710)(1,0);
    uintptr_t camera = ui ? *reinterpret_cast<uintptr_t*>(ui + 0x254) : 0;
    if (!camera) return;
    float saved[4]; auto limits = reinterpret_cast<float*>(camera + 0x4CC);
    memcpy(saved,limits,sizeof(saved));
    // SetCameraPosition's +303070 clamps in the camera's transformed XY frame.
    // Relax only those four limits during our synchronous position write. SetCameraBounds
    // also rebuilds minimap textures and transforms its inputs; persistent expansion and
    // reapplying getter values can produce a negative bitmap border on cinematic/Alt-Tab.
    limits[0]=limits[1]=-1000000.0f; limits[2]=limits[3]=1000000.0f;
    __try { wc3::SetCameraPosition(x,y); }
    __finally { memcpy(limits,saved,sizeof(saved)); }
}

void MapCameraGuard::Install(uintptr_t base, bool (*shouldBlock)(), HookInstaller hook) {
    blocked = shouldBlock;
    gameBase = base;
    const unsigned char cameraGetter[] = {0x8B,0x81,0x54,0x02,0x00,0x00,0xC3};
    const unsigned char minXGetter[] = {0xD9,0x80,0xD0,0x04,0x00,0x00};
    cameraLayoutVerified = !memcmp(reinterpret_cast<void*>(base+0x2F5ED0),cameraGetter,sizeof(cameraGetter)) &&
        !memcmp(reinterpret_cast<void*>(base+0x3B4C62),minXGetter,sizeof(minXGetter));
    wc3::Log("FPS transient camera clamp verified=%d; mission/minimap bounds retained",cameraLayoutVerified);
    // These 1.26a native entry points use the verified JASS cdecl signatures, including all camera setups.
    hook(reinterpret_cast<void*>(base + 0x3B48B0), reinterpret_cast<void*>(FieldHook), reinterpret_cast<void**>(&field));
    hook(reinterpret_cast<void*>(base + 0x3CD760), reinterpret_cast<void*>(FollowHook), reinterpret_cast<void**>(&follow));
    hook(reinterpret_cast<void*>(base + 0x3CD7B0), reinterpret_cast<void*>(OrientHook), reinterpret_cast<void**>(&orient));
    hook(reinterpret_cast<void*>(base + 0x3B45D0), reinterpret_cast<void*>(PositionHook), reinterpret_cast<void**>(&position));
    hook(reinterpret_cast<void*>(base + 0x3CD900), reinterpret_cast<void*>(SetupHook), reinterpret_cast<void**>(&setup));
    hook(reinterpret_cast<void*>(base + 0x3CD960), reinterpret_cast<void*>(SetupTimedHook), reinterpret_cast<void**>(&setupTimed));
    hook(reinterpret_cast<void*>(base + 0x3CD930), reinterpret_cast<void*>(SetupZHook), reinterpret_cast<void**>(&setupZ));
    hook(reinterpret_cast<void*>(base + 0x3CD990), reinterpret_cast<void*>(SetupTimedZHook), reinterpret_cast<void**>(&setupTimedZ));
    hook(reinterpret_cast<void*>(base + 0x3B4740), reinterpret_cast<void*>(PanTimedHook), reinterpret_cast<void**>(&panTimed));
    hook(reinterpret_cast<void*>(base + 0x3B47D0), reinterpret_cast<void*>(PanTimedZHook), reinterpret_cast<void**>(&panTimedZ));
    hook(reinterpret_cast<void*>(base + 0x3B46B0), reinterpret_cast<void*>(ResetHook), reinterpret_cast<void**>(&reset));
}
