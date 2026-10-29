#include "D3DHooks.hpp"
#include "D3DNativeUi.hpp"
#include <d3d9.h>
#include "../presentation/FullscreenView.hpp"
#include "../platform/WarcraftApi.hpp"
#include <MinHook.h>
#include <cstdint>
#include <cstring>
#include <utility>
#include <mutex>

// Direct3D 9 vtable indices. IDirect3D9 adds CheckDeviceFormatConversion over IDirect3D8, so the
// factory's CreateDevice sits at 16; IDirect3DDevice9 adds GetSwapChain and
// GetNumberOfSwapChains over the Direct3D 8 device, which pushes Reset to 16 and Present to 17.
// (Reading Direct3D 9 with the Direct3D 8 device indices silently patched GetSwapChain and
// GetNumberOfSwapChains and never anchored a frame.)
constexpr int kFactoryCreateDeviceSlot = 16;
// IDirect3D9Ex adds GetAdapterModeCountEx, EnumAdapterModesEx and GetAdapterDisplayModeEx over
// IDirect3D9, which puts CreateDeviceEx at 20.
constexpr int kFactoryCreateDeviceExSlot = 20;
constexpr int kDeviceResetSlot = 16;
constexpr int kDevicePresentSlot = 17;
// IDirect3DDevice9Ex adds SetConvolutionMonoKernel and ComposeRects over IDirect3DDevice9, which
// puts PresentEx at 121. A renderer that presents through it bypasses the plain Present hook.
constexpr int kDevicePresentExSlot = 121;
// After EndScene(42) and Clear(43): SetTransform is 44 and SetViewport 47. The two in between
// (GetTransform 45, MultiplyTransform 46) and the slot below each of them must never be hooked:
// a detour with the wrong parameter count pops the wrong number of stack bytes on x86 stdcall.
constexpr int kDeviceSetTransformSlot = 44;
constexpr int kDeviceSetViewportSlot = 47;
// The draw entry points (81-84) were measured for a scissor-driven UI filter, but hooking them
// by code address crashed inside this client: the wrapper's device exposes driver-generated
// thunks there (EIP landed in anonymous memory on the first probe). D3DNativeUi instead patches
// COM draw slots; these transform hooks retain world expansion and the fallback band clipping.
// A present can also arrive through the swap chain (IUnknown 0-2, then Present at 3), which is
// what most Direct3D 9 render loops use, so both entry points are anchored.
constexpr int kSwapChainPresentSlot = 3;
// Throwaway devices only need a valid window and surface size; nothing is ever shown.
constexpr int kProbeSize = 8;
// The client directory usually holds a wrapper (an ENB-style d3d9.dll) that keeps its own device
// objects, while the runtime it delegates to owns a second, distinct copy. Both are tracked.
constexpr int kSourceCount = 2;
// Source slots, shared by the early install thread and Install: the client-directory copy is what
// Warcraft resolves first, the system copy is the runtime it delegates to.
constexpr int kSearchSource = 0;
constexpr int kSystemSource = 1;
constexpr int kFactoryCount = 4;

namespace D3DHooks {
namespace {

using Create9 = IDirect3D9* (WINAPI*)(UINT);
using Create9Ex = HRESULT (WINAPI*)(UINT, IDirect3D9Ex**);
using CreateDevice9 = HRESULT (STDMETHODCALLTYPE*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
using CreateDeviceEx9 = HRESULT (STDMETHODCALLTYPE*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*, IDirect3DDevice9**);
using Present9 = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
using PresentEx9 = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD);
using Reset9 = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
using SwapPresent9 = HRESULT (STDMETHODCALLTYPE*)(IDirect3DSwapChain9*, const RECT*, const RECT*, HWND, const RGNDATA*);
using SetTransform9 = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DTRANSFORMSTATETYPE, const D3DMATRIX*);
using SetViewport9 = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*, const D3DVIEWPORT9*);

ProjectionCallback onProjection = nullptr;
// Raised by AnchorDevice and by the first present. The fallback probe waits for either before
// creating its throwaway device; the render-thread probe runs once a frame has been seen.
bool anchoredByCreate = false;
bool frameAnchorSeen = false;
// One probe attempt per loaded runtime.
bool probeAttempted[kSourceCount] = {};

// Frame blindness recovery. The game's real device can predate these hooks - the client-directory
// wrapper brings its device up while Warcraft starts - and on this client the first probe of that
// wrapper runtime can fail while the wrapper is still initialising. A single attempt then left the
// session anchored to a device that only ever presents the loading screen: no frames reached the
// tick, so the mod stayed dead (no HUD, F6 unresponsive) while the game itself looked fine. The
// probes therefore continue at a widening interval for as long as no frame arrives from any
// runtime, and every arriving frame resets the schedule.
DWORD lastPresentTick = 0;   // newest present seen through a hooked runtime
DWORD lastCreateTick = 0;    // newest game-side device creation, used to avoid overlapping it
DWORD blindProbeAt = 0;      // earliest tick for the next blind re-probe
DWORD blindProbeInterval = 1500;
unsigned blindProbeAttempts = 0;
// A device that predates the hooks never presents through them, so no frame is ever seen and the
// "no frames lately" test alone can never fire. This flag says Direct3D was already loaded when
// the hooks went in (an OpenGL session leaves it false and must never probe), and blindProbeStart
// holds off the first fetch until the engine has finished bringing its renderer up.
bool rendererPreloaded = false;
// Sampled by the early thread before it loads anything itself: -1 unknown, 0 not loaded, 1 loaded.
// Install must not confuse our own early load with the engine having brought Direct3D up, because
// that flag is what allows probing at all (an OpenGL session must never probe).
int rendererObservation = -1;
DWORD blindProbeStart = 0;
// A retry that lands inside the game's own CreateDevice crashes inside the wrapper (it is not
// thread safe), so a probe is never started within this window of an observed creation.
constexpr DWORD kProbeQuietMs = 1500;
// A session that presented a frame this recently is alive: building a throwaway device next to it
// would make the wrapper juggle two devices and crash the renderer, and the anchor it would learn
// is already in place.
constexpr DWORD kProbeLiveWindowMs = 3000;
// Uncapped before the first frame: the session is blind until a probe lands, so giving up would
// leave it dead for the whole match. Once frames flow the budget only covers a later stall.
constexpr unsigned kBlindProbeBudget = 24;
// Long enough that a normal load (21-28 s to the first frame) never triggers a fallback probe,
// while a session whose device predates the hooks still recovers within the first minute.
constexpr DWORD kFirstBlindProbeDelayMs = 40000;
constexpr DWORD kFirstBlindProbeIntervalMs = 5000;
// The renderer bring-up window is shorter than this in practice; the bootstrap probe uses the
// same figure before it touches a runtime. It must outlast the whole map load (the first frame
// lands around 27 s on this client), because a probe that is still in flight when the first
// frames arrive creates its device against a live wrapper and takes the process down.
constexpr DWORD kBootstrapWaitMs = 40000;

// Native UI band suppression, the Direct3D counterpart of the OpenGL geometry filter. The
// classic console strip and top bar are clipped with the scissor test, driven entirely from the
// projection transitions the engine already issues every frame. Full native submission filtering
// and portrait redirection belong to D3DNativeUi. The engine confines the world pass to the region
// above the console strip with its own rectangle and disables the test around its UI passes -
// so the wanted state is re-asserted per projection and the world pass is released from it.
bool uiSuppressRequested = false;
RECT uiWorldRect = {};
bool orthoProjection = false;   // the current projection transform is orthographic
bool scissorActive = false;     // our scissor is currently enabled on the device
bool overlayActive = false;     // the mod's own canvas is drawing; never clip it
bool worldViewportExpanded = false; // the frame's first perspective pass got the full client rect
// The engine's world pass and its verbatim projection repeat, recognised per frame: their
// scissor must be released, everything after them is clipped to the world region.
bool engineSawPerspective = false;
bool worldProjectionValid = false;
float worldProjection[16] = {};

// One loaded Direct3D 9 copy and the frame endpoints reachable through it. The functions are
// hooked by their code address rather than through a vtable slot: Warcraft and the overlay
// wrappers around it cache the pointers they resolved, so a slot written after the fact is never
// consulted, while every cached pointer still lands on the patched code.
struct Source {
    HMODULE module = nullptr;
    const char* label = "";
    Create9 create9 = nullptr;
    Create9Ex create9Ex = nullptr;
    CreateDevice9 createDevice = nullptr;
    CreateDeviceEx9 createDeviceEx = nullptr;
    Present9 devicePresent = nullptr;
    Present9 originalDevicePresent = nullptr;
    PresentEx9 devicePresentEx = nullptr;
    PresentEx9 originalDevicePresentEx = nullptr;
    Reset9 deviceReset = nullptr;
    Reset9 originalDeviceReset = nullptr;
    SwapPresent9 swapPresent = nullptr;
    SwapPresent9 originalSwapPresent = nullptr;
    SetTransform9 setTransform = nullptr;
    SetTransform9 originalSetTransform = nullptr;
    SetViewport9 setViewport = nullptr;
    SetViewport9 originalSetViewport = nullptr;
    // True once this runtime's frame endpoints are patched, i.e. presents through it reach us.
    bool frameEndpointsHooked = false;
};
Source sources[kSourceCount];

bool RuntimeAnchored(int source) {
    // Search-path and system slots can alias the same DLL after early capture; a second device teaches nothing.
    HMODULE module=sources[source].module;
    if(!module)return false;
    for(const auto& entry:sources)if(entry.module==module && entry.frameEndpointsHooked)return true;
    return false;
}

// A patched factory remembers which source it came from so its CreateDevice call still forwards
// to the right runtime. It is keyed by the factory's *vtable*, not by the factory object:
// Direct3DCreate9 hands out a new object on every call while every object of one runtime shares a
// single vtable, so an object-keyed table ran out after four calls and from then on every
// CreateDevice - including the engine's own - was refused with D3DERR_INVALIDCALL. Keying by
// vtable keeps exactly one entry per runtime and covers factory objects created before the hooks,
// because the slot they call through is that same shared table.
struct PatchedFactory { void** vtable = nullptr; int source = -1; };
PatchedFactory patchedFactories[kFactoryCount];


PresentCallback callback;
bool installed = false;
HWND window = nullptr;
HWND probeWindow = nullptr;
IDirect3DDevice9* activeDevice = nullptr;
// Set while ProbeDevice builds its throwaway device. The creation still passes through the patched
// factory, so the detour has to tell a probe device from the engine's: only the engine's may become
// the frame device. Remembering a probe device here leaves a dangling pointer the moment the probe
// releases it, and the next swap-chain present then draws through freed memory (this crashed inside
// the client's d3dx9 wrapper right after a probe that overlapped the first live frames).
bool probing = false;
IDirect3DDevice9* probeDevice = nullptr;
// Most recent viewport the device was given; a projection is only trusted as the world's when
// this covers the whole client, mirroring how the OpenGL path recognises its world batch.
D3DVIEWPORT9 lastViewport = {};

bool FrameSize(int& width, int& height) {
    RECT client = {};
    // In fullscreen the client rectangle equals the back buffer; in windowed mode it is the
    // actual game window. A destroyed/minimized window yields no size and skips the frame.
    if (window && GetClientRect(window, &client) && client.right > 0 && client.bottom > 0) {
        width = client.right; height = client.bottom;
        return true;
    }
    return false;
}

// Largest visible top-level window of this process: the game's render window. Warcraft is already
// running when these hooks install, so its device creation (and the window it was given) was
// never observed and the window has to be found instead of remembered.
struct WindowSearch { DWORD pid = 0; HWND best = nullptr; LONG area = 0; };
BOOL CALLBACK FindWindowProc(HWND candidate, LPARAM parameter) {
    WindowSearch* search = reinterpret_cast<WindowSearch*>(parameter);
    DWORD pid = 0;
    GetWindowThreadProcessId(candidate, &pid);
    if (pid != search->pid || !IsWindowVisible(candidate)) return TRUE;
    RECT rect = {};
    if (!GetClientRect(candidate, &rect)) return TRUE;
    LONG area = (rect.right - rect.left) * (rect.bottom - rect.top);
    if (area > search->area) { search->area = area; search->best = candidate; }
    return TRUE;
}
HWND ResolveWindow() {
    if (window && IsWindow(window)) return window;
    WindowSearch search;
    search.pid = GetCurrentProcessId();
    EnumWindows(FindWindowProc, reinterpret_cast<LPARAM>(&search));
    window = search.best;
    return window;
}

bool ProbeDevice(int source);
bool AnchorDevice(int source, IDirect3DDevice9* device, const char* origin);
DWORD ProcessUptimeMs();

// Builds throwaway devices through one runtime until one of them anchors. Two host windows are
// tried: the invisible helper first because it cannot disturb the engine, then the engine's own
// window, since a wrapper layer may refuse to build a device for a window it never saw the game
// render into. Whichever device comes up is anchored and released immediately.
void TryWindowedProbe(IDirect3D9* factory, Source& entry, int source, HWND helper, HWND engine, const DWORD* behaviors, bool& anchored) {
    HWND hosts[2] = {helper, engine};
    for (int host = 0; host < 2 && !anchored; ++host) {
        if (!hosts[host]) continue;
        // The creation itself can take a second inside the wrapper; frames may have started while
        // the previous attempt was in flight, in which case the session is no longer blind.
        if (host > 0 && frameAnchorSeen && GetTickCount() - lastPresentTick < kProbeLiveWindowMs) break;
        D3DPRESENT_PARAMETERS present = {};
        present.Windowed = TRUE;
        present.SwapEffect = D3DSWAPEFFECT_DISCARD;
        present.hDeviceWindow = hosts[host];
        present.BackBufferFormat = D3DFMT_UNKNOWN;
        present.BackBufferWidth = kProbeSize;
        present.BackBufferHeight = kProbeSize;
        present.BackBufferCount = 1;
        present.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        for (DWORD behavior = 0; behavior < 3; ++behavior) {
            IDirect3DDevice9* device = nullptr;
            HRESULT hr = factory->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hosts[host], behaviors[behavior], &present, &device);
            if (SUCCEEDED(hr) && device) {
                wc3::Log("D3D9 probe device created source=%s host=%s behavior=%X", entry.label, host ? "game" : "helper", behaviors[behavior]);
                anchored = AnchorDevice(source, device, "probe");
                if (probeDevice == device) probeDevice = nullptr;
                device->Release();
                wc3::Log("D3D9 probe device released source=%s anchored=%d", entry.label, anchored ? 1 : 0);
                break;
            }
            // A failing probe used to be silent; the HRESULT is what says whether the wrapper
            // refused the host window, the format or the behaviour.
            wc3::Log("D3D9 probe create failed source=%s host=%s behavior=%X hr=%08X", entry.label, host ? "game" : "helper", behaviors[behavior], static_cast<unsigned>(hr));
        }
    }
}

// A client-directory wrapper (ENBSeries) presents each frame twice: once from its own proxy device
// and once from the runtime it delegates to, both on the render thread and neither nested in the
// other. Drawing on both was not just a doubled overlay: the canvas changed device on every
// present, which released and re-uploaded every overlay resource (the "Overlay resources
// forgotten" churn), halving the frame rate and making the HUD flicker. The overlay therefore
// stays on one device - the system runtime once it presents, because that is the last draw before
// the flip - and only hands over when the chosen one has stopped presenting.
struct OverlayDeviceChoice { IDirect3DDevice9* device = nullptr; bool system = false; DWORD tick = 0; };
OverlayDeviceChoice overlayChoice;
constexpr DWORD kOverlayHandoverMs = 1000;

// True when this present is the one the overlay should draw into.
bool SelectOverlayDevice(IDirect3DDevice9* device, const char* label) {
    DWORD now = GetTickCount();
    bool system = label && strcmp(label, "system") == 0;
    if (!overlayChoice.device) {
        overlayChoice.device = device;
        overlayChoice.system = system;
        overlayChoice.tick = now;
        return true;
    }
    if (device == overlayChoice.device) {
        overlayChoice.tick = now;
        return true;
    }
    // A system-runtime device supersedes the wrapper's proxy: its present is the one that reaches
    // the screen, so the HUD is drawn after the wrapper's post-processing instead of inside it.
    bool stale = now - overlayChoice.tick > kOverlayHandoverMs;
    if (!system || overlayChoice.system) {
        if (!stale) return false;
    }
    wc3::Log("D3D9 overlay device %p -> %p (%s)", overlayChoice.device, device, stale ? "silent" : "system");
    overlayChoice.device = device;
    overlayChoice.system = system;
    overlayChoice.tick = now;
    return true;
}

void DrawFrame(IDirect3DDevice9* device, const char* label) {
    if (!device) return;
    // A probe device never presents a frame; presenting through one would draw into the throwaway
    // swap chain and, if the probe already released it, dereference freed memory.
    if (device == probeDevice) return;
    if (!SelectOverlayDevice(device, label)) return;
    D3DNativeUi::Attach(device);D3DNativeUi::Finish();
    frameAnchorSeen = true;
    // First present from a device this session has not drawn through yet: rendering that moved to
    // a device whose creation was never observed used to look like a silent stall.
    static IDirect3DDevice9* knownDevices[8] = {};
    bool knownDevice = false;
    for (IDirect3DDevice9* seen : knownDevices) {
        if (seen == device) { knownDevice = true; break; }
    }
    if (!knownDevice) {
        for (IDirect3DDevice9*& slot : knownDevices) {
            if (!slot) { slot = device; break; }
        }
        wc3::Log("D3D9 first present device=%p uptime=%lu", device, ProcessUptimeMs());
    }
    // A frame from any hooked runtime proves the anchor is live; a blind session re-probes at a
    // widening interval (see StartBlindProbeWatchdog) and this resets that schedule.
    lastPresentTick = GetTickCount();
    blindProbeInterval = 1500;
    blindProbeAttempts = 0;
    // A present ends the frame. Force the scissor off here: the next frame must start clean
    // even if the engine never re-issues a projection (RTS frames do not always do so), and
    // an FPS frame re-arms the clip at its first orthographic projection anyway.
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    scissorActive = false;
    // A stale ortho flag can never clip world draws that skip their SetTransform either.
    orthoProjection = false;
    engineSawPerspective = false;
    worldProjectionValid = false;
    worldViewportExpanded = false;
    // Two runtimes can present alternately (a wrapper delegating to the system copy), so the
    // report is remembered per label: one shared slot used to flip between "search" and "system"
    // and wrote two trace lines per frame.
    static const char* reportedLabels[4] = {};
    bool reportedLabel = false;
    for (const char* seen : reportedLabels) {
        if (seen == label) { reportedLabel = true; break; }
    }
    if (!reportedLabel) {
        for (const char*& slot : reportedLabels) {
            if (!slot) { slot = label; break; }
        }
        wc3::Log("D3D9 frame anchor active source=%s device=%p", label, device);
    }
    // The overlay ortho is built from the window client rect; a back buffer of a different size
    // (a wrapper or widescreen helper choosing its own surface) stretches every overlay pixel.
    static int reportedWidth = -1, reportedHeight = -1;
    IDirect3DSurface9* backBuffer = nullptr;
    if (SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) && backBuffer) {
        D3DSURFACE_DESC desc = {};
        if (SUCCEEDED(backBuffer->GetDesc(&desc)) && (int(desc.Width) != reportedWidth || int(desc.Height) != reportedHeight)) {
            reportedWidth = int(desc.Width); reportedHeight = int(desc.Height);
            RECT client = {};
            GetClientRect(window, &client);
            wc3::Log("D3D9 back buffer %ux%u client %ldx%ld", desc.Width, desc.Height, client.right, client.bottom);
        }
        backBuffer->Release();
    }
    int width = 0, height = 0;
    if (!callback || !ResolveWindow() || !FrameSize(width, height)) return;
    overlayActive = true;
    D3DNativeUi::Overlay(true);
    callback(device, window, width, height);
    D3DNativeUi::Overlay(false);
    overlayActive = false;
    // The canvas cleared the scissor for its own pass; the cache must say so or a later
    // ortho pass would believe the rect is still applied.
    scissorActive = false;
    // A probe used to run from here to learn a runtime that had not been anchored yet. It is gone:
    // a probe device built next to live frames makes the client wrapper juggle two devices and
    // crashes the renderer, and every runtime is now anchored by its own CreateDevice instead. A
    // session that never presents through a hooked runtime is recovered by the blind probe, which
    // only runs while no frame has arrived at all.
}

template<int S>
HRESULT STDMETHODCALLTYPE PresentDetour(IDirect3DDevice9* device, const RECT* source, const RECT* dest, HWND overrideWindow, const RGNDATA* dirty) {    Source& entry = sources[S];
    Present9 forward = entry.originalDevicePresent;
    // A second runtime copy can expose the very same function; the detour that was installed
    // first owns the trampoline, so it is borrowed before giving up.
    if (!forward)
        for (const Source& other : sources)
            if (other.devicePresent == entry.devicePresent && other.originalDevicePresent) forward = other.originalDevicePresent;
    if (!forward) return D3DERR_INVALIDCALL;
    DrawFrame(device, entry.label);
    return forward(device, source, dest, overrideWindow, dirty);
}
template<int S>
HRESULT STDMETHODCALLTYPE PresentExDetour(IDirect3DDevice9* device, const RECT* source, const RECT* dest, HWND overrideWindow, const RGNDATA* dirty, DWORD flags) {
    Source& entry = sources[S];
    PresentEx9 forward = entry.originalDevicePresentEx;
    if (!forward)
        for (const Source& other : sources)
            if (other.devicePresentEx == entry.devicePresentEx && other.originalDevicePresentEx) forward = other.originalDevicePresentEx;
    if (!forward) return D3DERR_INVALIDCALL;
    DrawFrame(device, entry.label);
    return forward(device, source, dest, overrideWindow, dirty, flags);
}
template<int S>
HRESULT STDMETHODCALLTYPE ResetDetour(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) {
    Source& entry = sources[S];
    Reset9 forward = entry.originalDeviceReset;
    if (!forward)
        for (const Source& other : sources)
            if (other.deviceReset == entry.deviceReset && other.originalDeviceReset) forward = other.originalDeviceReset;
    if (!forward) return D3DERR_INVALIDCALL;
    // D3DPOOL_MANAGED textures survive a Reset, so the overlay keeps its handles; the canvas
    // re-saves device state on the next frame.
    D3DNativeUi::BeforeReset(device);
    return forward(device, params);
}
template<int S>
HRESULT STDMETHODCALLTYPE SwapPresentDetour(IDirect3DSwapChain9* swapChain, const RECT* source, const RECT* dest, HWND overrideWindow, const RGNDATA* dirty) {
    Source& entry = sources[S];
    SwapPresent9 forward = entry.originalSwapPresent;
    if (!forward)
        for (const Source& other : sources)
            if (other.swapPresent == entry.swapPresent && other.originalSwapPresent) forward = other.originalSwapPresent;
    if (!forward) return D3DERR_INVALIDCALL;
    // The swap chain does not carry the device; it is asked once and remembered.
    if (!activeDevice && swapChain) swapChain->GetDevice(&activeDevice);
    DrawFrame(activeDevice, entry.label);
    return forward(swapChain, source, dest, overrideWindow, dirty);
}
template<int S>
HRESULT STDMETHODCALLTYPE SetTransformDetour(IDirect3DDevice9* device, D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) {
    Source& entry = sources[S];
    SetTransform9 forward = entry.originalSetTransform;
    if (!forward)
        for (const Source& other : sources)
            if (other.setTransform == entry.setTransform && other.originalSetTransform) forward = other.originalSetTransform;
    if (!forward) return D3DERR_INVALIDCALL;
    // The world projection is what world-anchored HUD art must be measured against; the caller
    // filters out the portrait and interface passes.
    if (state == D3DTS_PROJECTION && matrix) {
        // Direct3D orthographic matrices carry no perspective row (_34 = 0, _44 = 1); every
        // perspective world projection the engine issues has _34 = 1 and _44 = 0.
        orthoProjection = matrix->m[2][3] == 0.0f && matrix->m[3][3] == 1.0f;
        D3DNativeUi::Projection(device,orthoProjection);
        // The world pass binds its reduced viewport (above the console strip) before its
        // projection on some setups, and nothing re-issues the viewport afterwards. While the
        // FPS view hides the chrome, re-assert the full client rect right here so the world
        // renders into the console strip and top bar regions exactly like
        // FullscreenView::Expand achieves on the OpenGL backend. Only the frame's first
        // perspective pass is rewritten: later perspective passes (the console backdrop) must
        // keep their own mapping, and portraits plus the overlay's own passes stay untouched.
        if (!orthoProjection && uiSuppressRequested && !overlayActive && !worldViewportExpanded && window) {
            RECT client = {};
            D3DVIEWPORT9 current = {};
            if (GetClientRect(window, &client) && client.right > 0 && client.bottom > 0 &&
                SUCCEEDED(device->GetViewport(&current)) && current.Width && current.Height &&
                current.Width >= DWORD(client.right * 3 / 4) && current.Height >= DWORD(client.bottom / 2) &&
                (int(current.Width) < client.right || int(current.Height) < client.bottom)) {
                D3DVIEWPORT9 full = {0, 0, DWORD(client.right), DWORD(client.bottom), current.MinZ, current.MaxZ};
                lastViewport = full;
                worldViewportExpanded = true;
                // The original entry point, so the observation cache in our own detour is not
                // fed the expanded rectangle as if the engine had issued it.
                if (entry.originalSetViewport) entry.originalSetViewport(device, &full);
                else device->SetViewport(&full);
            }
        }
        if (onProjection) {
            const D3DVIEWPORT9 viewport = lastViewport;
            int box[4] = {int(viewport.X), int(viewport.Y), int(viewport.Width), int(viewport.Height)};
            onProjection(&matrix->m[0][0], box);
        }
    }
    HRESULT result = forward(device, state, matrix);
    // The engine manages the scissor itself: it enables it (with its own reduced world rect)
    // for the world pass and disables it around the UI passes, so a cached "armed" flag goes
    // stale within one frame and the world would stay confined above the console strip. The
    // wanted state is therefore re-asserted on every projection transition instead of trusting
    // the cache. The first engine perspective of the frame is the world (the overlay's own
    // weapon and preview projections run inside the present callback and never reach this
    // branch): its scissor is released so the world also renders into the hidden console strip
    // and top bar regions, and a verbatim repeat of the world projection stays released. Every
    // later pass - the orthographic chrome and the console backdrop, a perspective pass drawn
    // after the chrome that paints the strip black - is clipped to the world region.
    if (state == D3DTS_PROJECTION && matrix && !overlayActive && uiSuppressRequested) {
        bool worldRepeat = !orthoProjection && worldProjectionValid &&
            memcmp(worldProjection, &matrix->m[0][0], sizeof(worldProjection)) == 0;
        if (!orthoProjection && !engineSawPerspective) {
            memcpy(worldProjection, &matrix->m[0][0], sizeof(worldProjection));
            worldProjectionValid = true;
            engineSawPerspective = true;
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            scissorActive = false;
        } else if (worldRepeat) {
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            scissorActive = false;
        } else if (engineSawPerspective) {
            device->SetScissorRect(&uiWorldRect);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
            scissorActive = true;
            static bool announced = false;
            if (!announced) {
                announced = true;
                wc3::Log("D3D9 ui bands clipped rect=%ld,%ld-%ld,%ld", uiWorldRect.left, uiWorldRect.top, uiWorldRect.right, uiWorldRect.bottom);
            }
        }
    }
    return result;
}
template<int S>
HRESULT STDMETHODCALLTYPE SetViewportDetour(IDirect3DDevice9* device, const D3DVIEWPORT9* viewport) {
    Source& entry = sources[S];
    SetViewport9 forward = entry.originalSetViewport;
    if (!forward)
        for (const Source& other : sources)
            if (other.setViewport == entry.setViewport && other.originalSetViewport) forward = other.originalSetViewport;
    if (!forward) return D3DERR_INVALIDCALL;
    if (viewport) {
        D3DVIEWPORT9 outgoing = *viewport;
        RECT client = {};
        if (window && GetClientRect(window, &client) && client.right > 0 && client.bottom > 0) {
            D3DNativeUi::Viewport(device,outgoing,client.right,client.bottom);
            // Mirror the OpenGL submission: the engine's world viewport is bottom-anchored above
            // the console strip, so the strip region would stay unrendered once the chrome is
            // hidden. Record the strip from the engine's own rectangle, then expand the world
            // viewport over the whole client exactly like FullscreenView::Expand does on GL.
            // Small viewports (portraits, minimap models) and fullscreen UI stay untouched.
            bool worldSized = viewport->Width >= DWORD(client.right * 3 / 4) && viewport->Height >= DWORD(client.bottom / 2);
            if (worldSized && uiSuppressRequested) {
                // D3D owns its viewport observation; GL's FullscreenView keeps its existing contract.
                if (int(viewport->Width) < client.right || int(viewport->Height) < client.bottom) {
                    outgoing.X = 0; outgoing.Y = 0;
                    outgoing.Width = DWORD(client.right); outgoing.Height = DWORD(client.bottom);
                }
            }
        }
        lastViewport = outgoing;
        return forward(device, &outgoing);
    }
    return forward(device, viewport);
}
Present9 const kPresentDetours[kSourceCount] = {&PresentDetour<0>, &PresentDetour<1>};
PresentEx9 const kPresentExDetours[kSourceCount] = {&PresentExDetour<0>, &PresentExDetour<1>};
Reset9 const kResetDetours[kSourceCount] = {&ResetDetour<0>, &ResetDetour<1>};
SwapPresent9 const kSwapPresentDetours[kSourceCount] = {&SwapPresentDetour<0>, &SwapPresentDetour<1>};
SetTransform9 const kSetTransformDetours[kSourceCount] = {&SetTransformDetour<0>, &SetTransformDetour<1>};
SetViewport9 const kSetViewportDetours[kSourceCount] = {&SetViewportDetour<0>, &SetViewportDetour<1>};

// Code-hooks a single endpoint. Re-hooking an address another source already claimed is not an
// error: both detours forward through the one trampoline MinHook created.
bool HookEndpoint(void* target, void* detour, void** trampoline) {
    if (!target) return false;
    MH_STATUS created = MH_CreateHook(target, detour, trampoline);
    if (created == MH_ERROR_ALREADY_CREATED) return true;
    if (created != MH_OK) return false;
    return MH_EnableHook(target) == MH_OK;
}

// Replaces one COM vtable slot (a function pointer stored at vtable[index]) with our handler.
// The slot page is made writable briefly; the original pointer is kept to forward through.
// MinHook is not used here because it patches code bytes, not data slots.
template<typename T>
bool PatchVtableSlot(void* object, int index, T replacement, T* original) {
    if (!object) return false;
    void** vtable = *reinterpret_cast<void***>(object);
    if (!vtable || !vtable[index]) return false;
    if (!*original) *original = reinterpret_cast<T>(vtable[index]);
    if (vtable[index] == reinterpret_cast<void*>(replacement)) return true;
    DWORD previous = 0;
    if (!VirtualProtect(&vtable[index], sizeof(void*), PAGE_READWRITE, &previous)) return false;
    vtable[index] = reinterpret_cast<void*>(replacement);
    VirtualProtect(&vtable[index], sizeof(void*), previous, &previous);
    return true;
}

int SourceOfFactoryVtable(void** vtable) {
    if (!vtable) return -1;
    for (const PatchedFactory& entry : patchedFactories)
        if (entry.vtable && entry.vtable == vtable) return entry.source;
    return -1;
}
int SourceOf(IDirect3D9* factory) {
    if (!factory) return -1;
    return SourceOfFactoryVtable(*reinterpret_cast<void***>(factory));
}
// Reserves the vtable entry before its slot is written, so a factory can never reach the detour
// without a source. A full table is a programming error now that entries are per runtime; it is
// reported instead of silently dropping the patch.
bool RememberFactory(void** vtable, int source) {
    for (PatchedFactory& entry : patchedFactories) {
        if (entry.vtable != vtable) continue;
        if (entry.source < 0) entry.source = source;
        return true;
    }
    for (PatchedFactory& entry : patchedFactories) {
        if (entry.vtable) continue;
        entry.vtable = vtable;
        entry.source = source;
        return true;
    }
    return false;
}

IDirect3D9* WINAPI Create9Detour0(UINT version);
IDirect3D9* WINAPI Create9Detour1(UINT version);

HRESULT STDMETHODCALLTYPE CreateDevice9Hook(IDirect3D9* factory, UINT adapter, D3DDEVTYPE type, HWND focus, DWORD behavior, D3DPRESENT_PARAMETERS* params, IDirect3DDevice9** device);

// Anchors the endpoints of a live device. A device created after these hooks install reports its
// own functions, which is how a runtime whose probe failed is still covered.
bool AnchorDevice(int source, IDirect3DDevice9* device, const char* origin) {
    if (!device || source < 0) return false;
    if (strcmp(origin, "create") == 0) anchoredByCreate = true;
    Source& entry = sources[source];
    void** vtable = *reinterpret_cast<void***>(device);
    if (!vtable) return false;
    entry.devicePresent = reinterpret_cast<Present9>(vtable[kDevicePresentSlot]);
    entry.deviceReset = reinterpret_cast<Reset9>(vtable[kDeviceResetSlot]);
    bool present = HookEndpoint(reinterpret_cast<void*>(entry.devicePresent), reinterpret_cast<void*>(kPresentDetours[source]), reinterpret_cast<void**>(&entry.originalDevicePresent));
    bool reset = HookEndpoint(reinterpret_cast<void*>(entry.deviceReset), reinterpret_cast<void*>(kResetDetours[source]), reinterpret_cast<void**>(&entry.originalDeviceReset));
    // An Ex device can present through PresentEx, which lives far outside the plain interface and
    // bypasses the Present hook entirely; it is hooked whenever the device exposes it.
    IDirect3DDevice9Ex* deviceEx = nullptr;
    if (SUCCEEDED(device->QueryInterface(__uuidof(IDirect3DDevice9Ex), reinterpret_cast<void**>(&deviceEx))) && deviceEx) {
        void** exVtable = *reinterpret_cast<void***>(deviceEx);
        if (exVtable && exVtable[kDevicePresentExSlot]) {
            entry.devicePresentEx = reinterpret_cast<PresentEx9>(exVtable[kDevicePresentExSlot]);
            HookEndpoint(exVtable[kDevicePresentExSlot], reinterpret_cast<void*>(kPresentExDetours[source]), reinterpret_cast<void**>(&entry.originalDevicePresentEx));
        }
        deviceEx->Release();
    }
    // The swap chain exposes a second present path; a device without one is still usable.
    IDirect3DSwapChain9* swapChain = nullptr;
    if (SUCCEEDED(device->GetSwapChain(0, &swapChain)) && swapChain) {
        void** chainVtable = *reinterpret_cast<void***>(swapChain);
        if (chainVtable) {
            entry.swapPresent = reinterpret_cast<SwapPresent9>(chainVtable[kSwapChainPresentSlot]);
            HookEndpoint(reinterpret_cast<void*>(entry.swapPresent), reinterpret_cast<void*>(kSwapPresentDetours[source]), reinterpret_cast<void**>(&entry.originalSwapPresent));
        }
        swapChain->Release();
    }
    entry.setTransform = reinterpret_cast<SetTransform9>(vtable[kDeviceSetTransformSlot]);
    entry.setViewport = reinterpret_cast<SetViewport9>(vtable[kDeviceSetViewportSlot]);
    bool transform = HookEndpoint(reinterpret_cast<void*>(entry.setTransform), reinterpret_cast<void*>(kSetTransformDetours[source]), reinterpret_cast<void**>(&entry.originalSetTransform));
    bool viewport = HookEndpoint(reinterpret_cast<void*>(entry.setViewport), reinterpret_cast<void*>(kSetViewportDetours[source]), reinterpret_cast<void**>(&entry.originalSetViewport));
    if (present) entry.frameEndpointsHooked = true;
    wc3::Log("D3D9 anchored source=%s origin=%s vtable=%p present=%d reset=%d swapPresent=%p transform=%d viewport=%d",
        entry.label, origin, vtable, present ? 1 : 0, reset ? 1 : 0, entry.swapPresent, transform ? 1 : 0, viewport ? 1 : 0);
    return present;
}

// Shared tail of both creation hooks: a probe's device is anchored but never remembered, the
// engine's own device becomes the frame device.
void AdoptCreatedDevice(int source, IDirect3DDevice9* device, UINT adapter, HWND focus, D3DPRESENT_PARAMETERS* params, const char* kind) {
    if (probing) {
        AnchorDevice(source, device, "probe");
        probeDevice = device;
        wc3::Log("D3D9 probe device anchored via patched factory source=%s device=%p", sources[source].label, device);
        return;
    }
    AnchorDevice(source, device, "create");
    activeDevice = device;
    // The probe's own window must never become the frame size source.
    if (focus && focus != probeWindow) window = focus;
    wc3::Log("D3D9 device%s created source=%s window=%p adapter=%u windowed=%d uptime=%lu",
        kind, sources[source].label, window, adapter, (params && params->Windowed) ? 1 : 0, ProcessUptimeMs());
}

HRESULT STDMETHODCALLTYPE CreateDevice9Hook(IDirect3D9* factory, UINT adapter, D3DDEVTYPE type, HWND focus, DWORD behavior, D3DPRESENT_PARAMETERS* params, IDirect3DDevice9** device) {
    int source = SourceOf(factory);
    if (source < 0 || !sources[source].createDevice) {
        // The patch can land on a factory we never saw exported - a wrapper that shares one
        // vtable would hit this - and refusing silently would break the engine's own device.
        // Log it (rate limited) so the condition is visible before deciding how to forward it.
        static DWORD lastUnknownFactoryTrace = 0;
        DWORD now = GetTickCount();
        if (now - lastUnknownFactoryTrace >= 1000) {
            lastUnknownFactoryTrace = now;
            wc3::Log("D3D9 CreateDevice on an unrecognized factory; refused");
        }
        return D3DERR_INVALIDCALL;
    }
    HRESULT result = sources[source].createDevice(factory, adapter, type, focus, behavior, params, device);
    lastCreateTick = GetTickCount();
    if (FAILED(result) && !probing) {
        // The engine's failed creations are part of the startup story (a wrapper that refuses a
        // mode, a second adapter); without them a lost device is invisible in the log.
        wc3::Log("D3D9 CreateDevice failed source=%s hr=%08X windowed=%d uptime=%lu",
            sources[source].label, static_cast<unsigned>(result), (params && params->Windowed) ? 1 : 0, ProcessUptimeMs());
    }
    if (SUCCEEDED(result) && device && *device) {
        // The probe's own device is anchored like any other (its vtable is the one the engine's
        // device will call) but it is not the frame device: it is released right after the probe
        // and must never be remembered, and its creation must not count as the engine coming up.
        AdoptCreatedDevice(source, *device, adapter, focus, params, "");
    }
    return result;
}

HRESULT STDMETHODCALLTYPE CreateDeviceEx9Hook(IDirect3D9* factory, UINT adapter, D3DDEVTYPE type, HWND focus, DWORD behavior, D3DPRESENT_PARAMETERS* params, D3DDISPLAYMODEEX* mode, IDirect3DDevice9** device) {
    int source = SourceOf(factory);
    if (source < 0 || !sources[source].createDeviceEx) {
        static DWORD lastUnknownFactoryTrace = 0;
        DWORD now = GetTickCount();
        if (now - lastUnknownFactoryTrace >= 1000) {
            lastUnknownFactoryTrace = now;
            wc3::Log("D3D9 CreateDeviceEx on an unrecognized factory; refused");
        }
        return D3DERR_INVALIDCALL;
    }
    HRESULT result = sources[source].createDeviceEx(factory, adapter, type, focus, behavior, params, mode, device);
    lastCreateTick = GetTickCount();
    if (FAILED(result) && !probing) {
        wc3::Log("D3D9 CreateDeviceEx failed source=%s hr=%08X windowed=%d uptime=%lu",
            sources[source].label, static_cast<unsigned>(result), (params && params->Windowed) ? 1 : 0, ProcessUptimeMs());
    }
    if (SUCCEEDED(result) && device && *device) {
        AdoptCreatedDevice(source, *device, adapter, focus, params, "(ex)");
    }
    return result;
}

// One patch per runtime: every factory object of a runtime shares this vtable, so a second patch
// would capture our own detour as the "original" and recurse forever.
void PatchFactory(IDirect3D9* factory, int source) {
    Source& entry = sources[source];
    void** vtable = *reinterpret_cast<void***>(factory);
    if (!vtable || SourceOfFactoryVtable(vtable) >= 0) return;
    wc3::Log("D3D9 factory created source=%s vtable=%p uptime=%lu", entry.label, vtable, ProcessUptimeMs());
    if (!RememberFactory(vtable, source)) {
        wc3::Log("D3D9 factory table exhausted; CreateDevice left unpatched source=%s vtable=%p", entry.label, vtable);
        return;
    }
    if (!PatchVtableSlot(factory, kFactoryCreateDeviceSlot, CreateDevice9Hook, &entry.createDevice)) {
        wc3::Log("D3D9 CreateDevice vtable slot patch failed source=%s vtable=%p", entry.label, vtable);
    }
    // An Ex factory is often used through CreateDeviceEx, whose slot sits past the plain interface
    // and would silently bypass the patch above.
    IDirect3D9Ex* ex = nullptr;
    if (SUCCEEDED(factory->QueryInterface(__uuidof(IDirect3D9Ex), reinterpret_cast<void**>(&ex))) && ex) {
        ex->Release();
        if (!PatchVtableSlot(factory, kFactoryCreateDeviceExSlot, CreateDeviceEx9Hook, &entry.createDeviceEx)) {
            wc3::Log("D3D9 CreateDeviceEx vtable slot patch failed source=%s vtable=%p", entry.label, vtable);
        }
    }
}

IDirect3D9* Create9Common(UINT version, int source) {
    Source& entry = sources[source];
    if (!entry.create9) return nullptr;
    IDirect3D9* factory = entry.create9(version);
    if (!factory) return nullptr;
    PatchFactory(factory, source);
    return factory;
}
IDirect3D9* WINAPI Create9Detour0(UINT version) { return Create9Common(version, 0); }
IDirect3D9* WINAPI Create9Detour1(UINT version) { return Create9Common(version, 1); }
Create9 const kCreateDetours[kSourceCount] = {Create9Detour0, Create9Detour1};

HRESULT WINAPI Create9ExCommon(UINT version, IDirect3D9** factory, int source) {
    Source& entry = sources[source];
    if (!entry.create9Ex) return D3DERR_INVALIDCALL;
    HRESULT result = entry.create9Ex(version, reinterpret_cast<IDirect3D9Ex**>(factory));
    if (SUCCEEDED(result) && factory && *factory) PatchFactory(*factory, source);
    return result;
}
HRESULT WINAPI Create9ExDetour0(UINT version, IDirect3D9** factory) { return Create9ExCommon(version, factory, 0); }
HRESULT WINAPI Create9ExDetour1(UINT version, IDirect3D9** factory) { return Create9ExCommon(version, factory, 1); }
using Create9ExDetour = HRESULT (WINAPI*)(UINT, IDirect3D9**);
Create9ExDetour const kCreateExDetours[kSourceCount] = {Create9ExDetour0, Create9ExDetour1};

bool HookCreate9(int source, HMODULE module, const char* label) {
    if (!module) return false;
    // Early capture and engine initialization can discover the same runtime concurrently.
    static std::mutex exportsMutex;
    std::lock_guard<std::mutex> lock(exportsMutex);
    Source& entry = sources[source];
    // The early install thread may already have claimed this export; re-hooking the same address
    // would fail and report the whole session as unhooked.
    if (entry.module == module && entry.create9) return true;
    entry.module = module;
    entry.label = label;
    // Search-path and system slots often refer to one DLL: reuse its actual forwarding trampoline.
    for(const Source& other:sources)if(&other!=&entry && other.module==module && other.create9) {
        entry.create9=other.create9;entry.create9Ex=other.create9Ex;return true;
    }
    void* create9 = reinterpret_cast<void*>(GetProcAddress(module, "Direct3DCreate9"));
    if (!create9) {
        wc3::Log("D3D9 Create9 export missing source=%s module=%p", label, module);
        return false;
    }
    MH_STATUS created = MH_CreateHook(create9, reinterpret_cast<void*>(kCreateDetours[source]), reinterpret_cast<void**>(&entry.create9));
    // A second source can be the very same module (a launcher preloading the wrapper); the hook
    // already installed there is the one that forwards, so it counts as a success.
    if (created == MH_ERROR_ALREADY_CREATED) return entry.create9 != nullptr;
    if (created != MH_OK || MH_EnableHook(create9) != MH_OK) {
        wc3::Log("D3D9 Create9 hook failed source=%s module=%p", label, module);
        return false;
    }
    // The Ex export is a separate entry point that renderers and wrappers can prefer; its
    // factories need the same vtable patch, so it is claimed whenever it exists.
    void* create9Ex = reinterpret_cast<void*>(GetProcAddress(module, "Direct3DCreate9Ex"));
    if (create9Ex && entry.create9Ex == nullptr) {
        if (MH_CreateHook(create9Ex, reinterpret_cast<void*>(kCreateExDetours[source]), reinterpret_cast<void**>(&entry.create9Ex)) == MH_OK)
            MH_EnableHook(create9Ex);
    }
    return true;
}

// Warcraft resolves Direct3DCreate9 while it brings up its renderer, which happens before this
// plugin's init thread runs, so the game's device usually exists already and no CreateDevice call
// is left to observe. A throwaway device is created through the runtime instead: its vtable
// yields the exact Present/Reset/swap-chain functions the running device calls, which are then
// code-hooked. The probe window is never shown and the device is released immediately.
bool ProbeDevice(int source) {    Source& entry = sources[source];
    if (!entry.module) return false;
    // Nothing left to learn: this runtime's presents already reach the frame tick.
    if (RuntimeAnchored(source)) return true;
    // An exclusive device normally stops presenting while inactive. Never compete with its native restore/reset.
    HWND engine=ResolveWindow();
    if(!engine || IsIconic(engine) || !IsWindowVisible(engine) || GetForegroundWindow()!=engine)return false;
    // A live session must never be probed. Building a second device through the client's wrapper
    // while the engine is presenting corrupts the wrapper's own device (it is not thread safe) and
    // the process dies inside d3dx9 on the next effect pass - and the throwaway device would also
    // replace the frame device with a pointer that is released moments later. Once frames flow the
    // anchor is already live, so there is nothing a probe could add.
    if (frameAnchorSeen && GetTickCount() - lastPresentTick < kProbeLiveWindowMs) return false;
    auto create9 = reinterpret_cast<Create9>(GetProcAddress(entry.module, "Direct3DCreate9"));
    if (!create9) { wc3::Log("D3D9 probe source=%s: no Create9 export", entry.label); return false; }
    if (!probeWindow) probeWindow = CreateWindowExA(0, "STATIC", "WarcraftCS D3D probe", WS_POPUP,
        0, 0, kProbeSize, kProbeSize, nullptr, nullptr, nullptr, nullptr);
    if (!probeWindow) { wc3::Log("D3D9 probe source=%s: probe window unavailable", entry.label); return false; }
    bool anchored = false;
    IDirect3D9* factory = create9(D3D_SDK_VERSION);
    if (!factory) { wc3::Log("D3D9 probe source=%s: Direct3DCreate9 returned null", entry.label); return false; }
    // Two host windows are tried. The invisible helper goes first because it cannot disturb the
    // engine; the engine's own window is the fallback, since a wrapper layer may refuse to build
    // a device for a window it never saw the game render into. Whichever device comes up is
    // anchored and then released immediately, exactly as before.
    // The window is resolved here as well: in the blind case the engine built its device before
    // these hooks and no frame ever ran, so nothing has filled it in yet - and without it this
    // fallback would be the null pointer and the session would stay blind.
    HWND engineWindow = window ? window : ResolveWindow();
    const DWORD behaviors[] = {D3DCREATE_SOFTWARE_VERTEXPROCESSING, D3DCREATE_HARDWARE_VERTEXPROCESSING, D3DCREATE_MIXED_VERTEXPROCESSING};
    probing = true;
    TryWindowedProbe(factory, entry, source, probeWindow, engineWindow, behaviors, anchored);
    // Exclusive fullscreen cannot be recovered this way: the engine's device holds the adapter
    // exclusively, so *no* device can be created in this process - windowed or fullscreen, both
    // come back D3DERR_DEVICELOST - and relieving the mode by minimizing the window was measured
    // crashing the engine itself (FATAL ERROR during its restore). A fullscreen session whose
    // device predates these hooks stays blind by design; the readable verdict below says so.
    probing = false;
    probeDevice = nullptr;
    factory->Release();
    wc3::Log("D3D9 probe source=%s module=%p anchored=%d", entry.label, entry.module, anchored ? 1 : 0);
    return anchored;
}

// The system copy is probed as well: a wrapper in the client directory owns its own device
// objects, so only the runtime underneath exposes the functions its delegated presents call.
HMODULE LoadSystemD3D9() {
    char path[MAX_PATH] = {};
    UINT length = GetSystemDirectoryA(path, MAX_PATH);
    if (!length || length + 12 >= MAX_PATH) return nullptr;
    strcat_s(path, "\\d3d9.dll");
    return LoadLibraryA(path);
}

// The engine resolves the client-directory runtime, while this module loads the system copy itself
// (early, to claim Create9), so a loaded handle that is not that copy means Direct3D renders this
// session. Handles compared, not names: both copies are called d3d9.dll.
bool EngineLoadedDirect3D() {
    HMODULE loaded = GetModuleHandleA("d3d9.dll");
    return loaded && loaded != sources[kSystemSource].module;
}

// Milliseconds since this process started. Log timestamps are wall clock, while every question in
// this file is "before or after the engine created its device", which is relative to process start.
DWORD ProcessUptimeMs() {
    FILETIME created = {}, exited = {}, kernel = {}, user = {};
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return 0;
    FILETIME now = {};
    GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER begin = {};
    begin.LowPart = created.dwLowDateTime;
    begin.HighPart = created.dwHighDateTime;
    ULARGE_INTEGER end = {};
    end.LowPart = now.dwLowDateTime;
    end.HighPart = now.dwHighDateTime;
    return static_cast<DWORD>((end.QuadPart - begin.QuadPart) / 10000);
}

// Claims the Create9 exports during process startup, long before the module's engine work (native
// bind, engine hooks, Install) is done. Warcraft loads d3d9 and builds its device inside the first
// few hundred ms of its life, so waiting for the load and hooking afterwards is measurably too
// late: the device exists, presents bypass every hook, and in exclusive fullscreen no second
// device can be created for a probe (D3DERR_DEVICELOST) - the session stayed blind for the whole
// match. Both runtimes are therefore loaded and hooked here, before the engine asks for d3d9 at
// all; the engine's own CreateDevice call then anchors the very device it presents with.
DWORD WINAPI EarlyInstallThread(void*) {
    // Sampled before anything is loaded here: only the engine's own load means it renders through
    // Direct3D, and that verdict decides whether a session may ever be probed.
    rendererObservation = GetModuleHandleA("d3d9.dll") ? 1 : 0;
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        return 0;
    }
    // The client-directory copy first: pre-loading it means the engine's own d3d9 request resolves
    // to this module, and its export is already claimed when the renderer initializes.
    char directory[MAX_PATH] = {};
    bool hooked = false;
    if (GetModuleFileNameA(nullptr, directory, MAX_PATH)) {
        if (char* slash = strrchr(directory, '\\')) {
            strcpy_s(slash + 1, MAX_PATH - (slash + 1 - directory), "d3d9.dll");
            if (GetFileAttributesA(directory) != INVALID_FILE_ATTRIBUTES) {
                hooked = HookCreate9(kSearchSource, LoadLibraryA(directory), "search");
            }
        }
    }
    // The system runtime is hooked as well: the wrapper delegates its device creation to it, so
    // both copies see the engine's factories no matter which one the engine resolves.
    HMODULE system = LoadSystemD3D9();
    if (system) hooked = HookCreate9(kSystemSource, system, "system") || hooked;
    wc3::Log("D3D9 early hooks installed=%d searchPreloaded=%d system=%p uptime=%lu",
        hooked ? 1 : 0, sources[kSearchSource].module ? 1 : 0, system, ProcessUptimeMs());
    return 0;
}

// Watches for a session whose anchor receives no frames at all - the game renders through a device
// this process never saw created - and re-probes the runtimes that are still unpatched until one
// streams frames. Attempts widen from 1.5 s to a minute apart and never overlap the game's own
// device creation, and each arriving frame resets the schedule, so a healthy session never probes.
DWORD WINAPI BlindProbeWatchdog(void*) {
    for (;;) {
        Sleep(500);
        // The engine can load its runtime after Install ran, so the Direct3D verdict is refreshed
        // here; an OpenGL session never matches and is never probed.
        if (!rendererPreloaded && EngineLoadedDirect3D()) {
            rendererPreloaded = true;
            blindProbeStart = GetTickCount() + kFirstBlindProbeDelayMs;
        }
        // Alt-Tab/minimize is not a lost hook. Leave a quiet window after focus returns for the engine's Reset.
        HWND engine=ResolveWindow();
        if(!engine || IsIconic(engine) || !IsWindowVisible(engine) || GetForegroundWindow()!=engine) {
            blindProbeAt=GetTickCount()+kProbeLiveWindowMs;
            continue;
        }
        if (!frameAnchorSeen) {
            // Not one frame has arrived: the engine's device was built before these hooks, and only
            // a Direct3D session can be recovered. Wait out the renderer bring-up first, because a
            // probe that lands inside the game's own CreateDevice faults inside the wrapper.
            if (!rendererPreloaded) continue;
            if (static_cast<LONG>(GetTickCount() - blindProbeStart) < 0) continue;
        } else {
            if (GetTickCount() - lastPresentTick < 2500) continue;  // frames are flowing
            if (blindProbeAttempts >= kBlindProbeBudget) continue;
        }
        if (GetTickCount() - lastCreateTick < kProbeQuietMs) continue;
        if (static_cast<LONG>(GetTickCount() - blindProbeAt) < 0) continue;
        blindProbeAt = GetTickCount() + blindProbeInterval;
        blindProbeInterval = blindProbeInterval < 60000 ? blindProbeInterval * 2 : 60000;
        ++blindProbeAttempts;
        // System runtime first (see the bootstrap note): the wrapper is probed only when the
        // system probe has already failed to start the frames, so a widening-interval beat that
        // recovers the session never reaches the fragile wrapper at all.
        for (int pass = kSourceCount; pass-- > 0;) {
            int source = pass;
            if (!sources[source].module || RuntimeAnchored(source)) continue;
            wc3::Log("D3D9 blind re-probe source=%s attempt=%u frames=%d", sources[source].label,
                blindProbeAttempts, frameAnchorSeen ? 1 : 0);
            ProbeDevice(source);
        }
        // One human-readable verdict per blind session: everything above is per-attempt noise.
        if (!frameAnchorSeen && blindProbeAttempts == 3)
            wc3::Log("D3D9 no frames captured yet; F6 presses are queued until the game renders through a hooked device. A fullscreen session cannot be attached at all - relaunch windowed (Start WarcraftCS (D3D9).cmd)");
    }
}

void StartBlindProbeWatchdog() {
    HANDLE thread = CreateThread(nullptr, 0, BlindProbeWatchdog, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
}

} // namespace

void StartEarlyInstall() {
    static bool started = false;
    if (started) return;
    started = true;
    HANDLE thread = CreateThread(nullptr, 0, EarlyInstallThread, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
}

bool Install(PresentCallback present, ProjectionCallback projection) {
    if (installed) return true;
    callback = std::move(present);
    onProjection = projection;
    // Diagnostic only: holding the hook install back reproduces the session where the engine has
    // already built its D3D9 device before the module gets here - exactly the case the probe
    // exists for. Never set during normal play.
    char delayBuffer[16] = {};
    if (GetEnvironmentVariableA("WCS_D3D_INSTALL_DELAY_MS", delayBuffer, sizeof(delayBuffer))) {
        int delay = atoi(delayBuffer);
        if (delay > 0 && delay < 60000) {
            wc3::Log("D3D9 hook install delayed %d ms (diagnostic)", delay);
            Sleep(static_cast<DWORD>(delay));
        }
    }
    // Only the engine's own load of the client-directory runtime marks a Direct3D session. This
    // module loads the system runtime itself (early, to claim Create9), so a plain handle check
    // would report Direct3D for an OpenGL session as well.
    if (rendererObservation < 0) rendererObservation = GetModuleHandleA("d3d9.dll") ? 1 : 0;
    HMODULE active = GetModuleHandleA("d3d9.dll");
    rendererPreloaded = rendererObservation == 1 || EngineLoadedDirect3D();
    // A session that never sees a frame has no "frames stopped" moment to react to, so the first
    // fallback attempt is scheduled from install time instead.
    blindProbeStart = GetTickCount() + kFirstBlindProbeDelayMs;
    blindProbeInterval = kFirstBlindProbeIntervalMs;
    HMODULE search = LoadLibraryA("d3d9.dll");
    HMODULE system = LoadSystemD3D9();
    // Both may already be claimed by the early thread; HookCreate9 reports those as success.
    bool hooked = HookCreate9(kSearchSource, search, "search");
    if (system && system != search) {
        if (HookCreate9(kSystemSource, system, "system")) hooked = true;
    }
    installed = hooked;
    wc3::Log("D3D9 hooks installed=%d renderer=%d preloaded=%d search=%p system=%p uptime=%lu", installed, active ? 1 : 0, rendererPreloaded ? 1 : 0, search, system, ProcessUptimeMs());
    if (!hooked) return installed;
    // Only a renderer that already loaded Direct3D needs the probe; an OpenGL frame never reaches
    // these functions and creating a device there would be a needless side effect. The ordinary
    // probe happens on the render thread inside the first presented frame (see DrawFrame). This
    // fallback covers the one case that never gets there: a device created before these hooks
    // installed, whose presents bypass every anchored function. It waits past the game's renderer
    // bring-up because a probe overlapping the game's CreateDevice crashes inside the wrapper.
    if (rendererPreloaded) {
        // The bootstrap probe runs during the load screen, before the game presents. Every loaded
        // runtime is tried, not just the system one: a wrapper that refuses the system runtime can
        // still build a device on the engine window, and leaving it untried used to cost the whole
        // session (the bootstrap's one attempt blocked every later probe of that runtime).
        struct WaitGuard { static DWORD WINAPI Start(void*) {
            // A device that predates these hooks exists long before this point, so a long wait
            // costs nothing in the case the bootstrap exists for and keeps the probe from ever
            // overlapping the game's own CreateDevice (which crashes inside the wrapper).
            for (DWORD waited = 0; waited < kBootstrapWaitMs && !anchoredByCreate && !frameAnchorSeen; waited += 100)
                Sleep(100);
            // The engine device that was created under the early hooks is the one presenting;
            // building throwaway devices next to it while the renderer is still coming up is the
            // known wrapper corruption case, and a probe could not teach this session anything.
            if (anchoredByCreate || frameAnchorSeen) return 0;
            for (int pass = kSourceCount; pass-- > 0;) {
                // System first, wrapper last: creating a throwaway device through the client's
                // wrapper is what kills the process (twice observed inside its d3dx9 effect pass),
                // while the wrapper delegates its own device creation to the system runtime, so an
                // anchored system usually starts the frames and the wrapper never needs probing.
                int source = pass;
                if (!sources[source].module || sources[source].frameEndpointsHooked || probeAttempted[source]) continue;
                probeAttempted[source] = true;
                wc3::Log("D3D9 bootstrap probe start source=%s anchored=%d", sources[source].label,
                    anchoredByCreate ? 1 : 0);
                ProbeDevice(source);
            }
            return 0;
        } };
        HANDLE thread = CreateThread(nullptr, 0, WaitGuard::Start, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    // A single failed probe must not leave the whole session blind: the watchdog keeps trying the
    // unpatched runtime while no frames arrive, and stands down the moment frames reach the tick.
    StartBlindProbeWatchdog();
    return installed;
}
bool Installed() { return installed; }

void SetUiSuppression(bool enabled, const RECT& worldRegion) {
    // A degenerate region (no strip measured yet, tiny window) must never produce an invalid
    // scissor call, which Direct3D would reject for the whole frame.
    bool usable = enabled && worldRegion.right > worldRegion.left && worldRegion.bottom > worldRegion.top;
    uiSuppressRequested = usable;
    if (usable) uiWorldRect = worldRegion;
}

} // namespace D3DHooks
