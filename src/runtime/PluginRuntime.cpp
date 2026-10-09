#include "PluginRuntime.hpp"
#include "../input/InputDispatcher.hpp"
#include "../platform/NativeFloatingText.hpp"
#include "../platform/WarcraftApi.hpp"
#include "../platform/DiagnosticLog.hpp"
#include "../application/ShooterController.hpp"
#include "../presentation/Overlay.hpp"
#include "../presentation/ReticleDiagnostics.hpp"
#include "../platform/MapCameraGuard.hpp"
#include "../platform/ActorRenderFilter.hpp"
#include "../platform/FpsCombatGuard.hpp"
#include "../presentation/FullscreenView.hpp"
#include "../presentation/FpsProjection.hpp"
#include <MinHook.h>
#include <algorithm>
#include <cstring>
#ifdef WCS_FLOATING_TEXT_TEST
#include "../../tests/FloatingTextScene.hpp"
#endif
#ifdef WCS_STATUS_TEST
#include "../../tests/StatusEffectScene.hpp"
#endif
#ifdef WCS_GAMEPLAY_TEST
#include "../../tests/RuneCombatScene.hpp"
#endif
#ifdef WCS_TREE_WHEEL_TEST
#include "../../tests/TreeAndWheelScene.hpp"
#endif

#ifdef WCS_SURFACE_TEST
#include "../../tests/WorldSurfaceScene.hpp"
#endif

static ShooterController controller;
static Overlay overlay;
static bool healthy = true;
static DWORD lastWorld = 0;
static char root[MAX_PATH];
static uintptr_t gameBase = 0;
using RenderWorld = int (__fastcall*)(uintptr_t, uintptr_t);
using Swap = BOOL (WINAPI*)(HDC, UINT);
using Pause = void (__cdecl*)(BOOL);
using Perspective = void (__fastcall*)(uintptr_t, uintptr_t, float, float, float, float);
using RenderUI = void (__fastcall*)(uintptr_t, uintptr_t);
static RenderWorld originalWorld = nullptr;
static Swap originalSwap = nullptr;
static Pause originalPause = nullptr;
static Perspective originalPerspective = nullptr;
static RenderUI originalUI = nullptr;
static WNDPROC originalWindow = nullptr;
static HWND gameWindow = nullptr;
static bool nativeUIPhase = false, overlayPass = false;
static uintptr_t mapUI = 0;
using DeleteContext = BOOL (WINAPI*)(HGLRC);
using Interface = void (__cdecl*)(BOOL, float*);
static DeleteContext originalDeleteContext = nullptr;
static Interface originalInterface = nullptr;
using DrawElements = void (APIENTRY*)(GLenum, GLsizei, GLenum, const void*);
static DrawElements originalDrawElements = nullptr;
using Viewport = void (APIENTRY*)(GLint, GLint, GLsizei, GLsizei);
static Viewport originalViewport = nullptr, originalScissor = nullptr;
static bool WorldRectangle(RECT& rect) {
    return healthy && controller.Visible() && gameWindow &&
        GetClientRect(gameWindow, &rect) && rect.right > 0 && rect.bottom > 0;
}
static void APIENTRY ViewportHook(GLint x, GLint y, GLsizei width, GLsizei height) {
    RECT rect={};
    // Expand only world GL submission; native portrait layout and its viewport stay untouched.
    if (WorldRectangle(rect)) FullscreenView::Expand(rect.right,rect.bottom,x,y,width,height);
    originalViewport(x,y,width,height);
}
static void APIENTRY ScissorHook(GLint x, GLint y, GLsizei width, GLsizei height) {
    RECT rect={};
    if (WorldRectangle(rect)) FullscreenView::Expand(rect.right,rect.bottom,x,y,width,height);
    originalScissor(x,y,width,height);
}

static InputDispatcher input(controller);
static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM key, LPARAM data) {
    // Window lifecycle explains focus, resizing and orderly exits without logging text input.
    if (message == WM_ACTIVATEAPP || message == WM_SIZE || message == WM_CLOSE || message == WM_DESTROY)
        wc3::Log("window event=%u value=%u size=%ux%u", message, unsigned(key), LOWORD(data), HIWORD(data));
    // Rendering owns focus resource recovery; input owns message translation and key consumption.
    if (message == WM_ACTIVATEAPP && key) overlay.RefreshAfterFocus();
    auto handled = input.Handle(window, message, key, data, healthy);
    return handled ? *handled : CallWindowProcA(originalWindow, window, message, key, data);
}
static uintptr_t CurrentUI() {
    using GetUI = uintptr_t (__fastcall*)(int, int);
    return reinterpret_cast<GetUI>(gameBase + 0x300710)(0, 0);
}
static void ObserveMap(uintptr_t ui) {
    if (mapUI == ui) return;
    // A real UI unload/replacement marks map boundaries; a rendering pause is only a focus pause.
    controller.ResetMap(); mapUI = ui;
    if (ui) healthy = true;
    wc3::Log("Map lifecycle: %s ui=%08X", ui ? "loaded" : "unloaded", ui);
}
static BOOL WINAPI DeleteContextHook(HGLRC context) {
    BOOL result = originalDeleteContext(context);
    if (result) overlay.ContextDeleted(context);
    return result;
}
static void __cdecl InterfaceHook(BOOL show, float* duration) {
    // Only map-script requests yield FPS control; the controller tags its own HUD requests.
    controller.InterfaceRequest(show != FALSE);
    originalInterface(show, duration);
}
static void SafeTick(uintptr_t ui) {
    // Disable the extension on a native fault, leaving the game callback intact.
    __try { controller.Tick(ui); }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        healthy = false; wc3::LogError("Controller fault %08X; disabled", GetExceptionCode());
        // A diagnostic/native failure must never keep FPS input or camera ownership locked over an exit dialog.
        __try { controller.RecoverFault(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { wc3::LogError("Native view restoration failed; input remains released"); }
    }
}
static int __fastcall WorldHook(uintptr_t ui, uintptr_t unused) {
    DWORD now = GetTickCount();
    lastWorld = now;
    // RenderWorld's this pointer is not CGameUI; query the real UI for cinematic flags.
    uintptr_t gameUI = CurrentUI(); ObserveMap(gameUI);
    if (healthy && gameUI) SafeTick(gameUI);
#ifdef WCS_FLOATING_TEXT_TEST
    if (healthy && gameUI) FloatingTextScene::Tick(controller,gameBase,root);
#endif
#ifdef WCS_STATUS_TEST
    // Real native spell casts are a separate opt-in oracle, excluded from ordinary builds.
    if (healthy && gameUI) StatusEffectScene::Tick(controller, gameBase, root);
#endif
#ifdef WCS_GAMEPLAY_TEST
    if (healthy && gameUI) RuneCombatScene::Tick(controller,gameBase,root);
#endif
#ifdef WCS_TREE_WHEEL_TEST
    // Dedicated tree/input oracles never run in ordinary release builds.
    if (healthy && gameUI) TreeAndWheelScene::Tick(controller,gameBase,root);
#endif
#ifdef WCS_SURFACE_TEST
    if (healthy && gameUI) WorldSurfaceScene::Tick(root);
#endif
    // Keep layout evaluation running while hiding its final graphics after the world pass.
    nativeUIPhase = false;
    // Presentation adapters consume an immutable camera snapshot after the simulation tick.
    WorldLabelView labelView;labelView.active=healthy && controller.Visible();
    if (labelView.active) {
        labelView.eye[0]=wc3::Real(wc3::GetCameraEyePositionX());
        labelView.eye[1]=wc3::Real(wc3::GetCameraEyePositionY());
        labelView.eye[2]=wc3::Real(wc3::GetCameraEyePositionZ());
        labelView.yaw=controller.ViewYaw();labelView.pitch=std::clamp(controller.ViewPitch(),-65.f,65.f);
        labelView.verticalFov=controller.ViewFov();
        RECT rect={};if (gameWindow && GetClientRect(gameWindow,&rect) && rect.bottom>0) labelView.aspect=float(rect.right)/rect.bottom;
        labelView.maximumDistance=controller.Settings().floatingTextDistance;
    }
    NativeFloatingText::SetView(labelView);
    // Suppress only the controlled actor during world drawing; menus, portraits and RTS still render it.
    ActorRenderFilter::Begin(healthy ? controller.ViewActor() : 0);
    int result = originalWorld(ui, unused);
    ActorRenderFilter::End();
    nativeUIPhase = true;
    return result;
}
static void __cdecl PauseHook(BOOL value) {
    // Log real pause transitions rather than repeated map requests for the same state.
    static BOOL previous = FALSE;
    if (previous != value) { wc3::Log("native pause=%d", value); previous = value; }
    controller.SetPaused(value != FALSE); originalPause(value);
}
static void __fastcall UIHook(uintptr_t frame, uintptr_t unused) {
    // Suppress classic console/cinematic backing only during live FPS; menus keep their UI.
    using GetUI = uintptr_t (__fastcall*)(int, int);
    uintptr_t ui = reinterpret_cast<GetUI>(gameBase + 0x300710)(0, 0);
    // UI drawing is filtered at GL submission, preserving layout and frame update callbacks.
    nativeUIPhase = ui && !*reinterpret_cast<int*>(ui + 0x258) && !*reinterpret_cast<int*>(ui + 0x260);
    originalUI(frame, unused);
}
static void APIENTRY DrawElementsHook(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    // Warcraft batches the console after world rendering, including cinematic backing textures.
    if (healthy && nativeUIPhase && !overlayPass && controller.Visible() && GetTickCount() - lastWorld < 500) return;
    originalDrawElements(mode, count, type, indices);
}
static void __fastcall PerspectiveHook(uintptr_t output, uintptr_t unused, float fov, float aspect, float nearZ, float farZ) {
    // Projection precedes world batching, so scope by the FPS camera's configured far distance.
    // Keep native FOV/aspect and portrait projection unchanged.
    nearZ=FpsProjection::NearPlane(healthy && controller.Visible() && GetTickCount()-lastWorld<500,nearZ,farZ);
    originalPerspective(output, unused, fov, aspect, nearZ, farZ);
}
static BOOL WINAPI SwapHook(HDC dc, UINT planes) {
    static DWORD previous = 0;
    static unsigned frames = 0;
    ++frames;
    ObserveMap(CurrentUI());
    HWND window = WindowFromDC(dc);
    if (!gameWindow && window && GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId()) {
        gameWindow = window;
        originalWindow = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
        controller.AttachWindow(window);
        wc3::Log("OpenGL window attached");
    }
    overlayPass = true;
    if (healthy && GetTickCount() - lastWorld < 500) overlay.Draw(dc, controller);
    // The reticle cannot reach its draw entry while native recovery or stale world frames suppress the overlay.
    else ReticleDiagnostics::Skip(healthy ? "world-frame-stale" : "controller-fault", 0, 0, GetTickCount() - lastWorld);
    overlayPass = false; nativeUIPhase = false;
    // Observe skipped frames too: a valid context alone does not establish an active FPS overlay.
    if (DiagnosticLog::Due(previous)) {
        RECT client = {}; if (window) GetClientRect(window, &client);
        wc3::Trace("render frames=%u healthy=%d fps=%d worldAgeMs=%lu context=%p focus=%d window=%ldx%ld buying=%d scope=%d",
            frames, healthy, controller.Visible(), GetTickCount() - lastWorld, wglGetCurrentContext(),
            window == GetForegroundWindow(), client.right, client.bottom, controller.Buying(), controller.Scoped());
        frames = 0;
    }
    BOOL result = originalSwap(dc, planes);
    if (!result) wc3::LogError("SwapLayerBuffers failed win32=%lu", GetLastError());
    return result;
}
static bool Hook(void* target, void* callback, void** original) {
    MH_STATUS create = MH_CreateHook(target, callback, original);
    MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : create;
    wc3::Log("hook target=%08X create=%d enable=%d", target, create, enable);
    return create == MH_OK && enable == MH_OK;
}
static DWORD WINAPI Initialize(void*) {
    HMODULE game = nullptr;
    for (int i = 0; i < 100 && !game; ++i) { game = GetModuleHandleA("Game.dll"); if (!game) Sleep(100); }
    // Load retention before opening so custom archive limits apply to the previous session too.
    DiagnosticLog::Configure(GameplaySettings::Load(std::string(root) + "\\WarcraftCS.ini").logging);
    wc3::OpenLog(root);
    SYSTEM_INFO system = {}; GetNativeSystemInfo(&system);
    wc3::Log("Session start version=%s source=%s compiled=%s %s nativeArch=%u pid=%lu",
        WCS_BUILD_VERSION, WCS_BUILD_SOURCE, __DATE__, __TIME__, system.wProcessorArchitecture, GetCurrentProcessId());
    using GetVersion = LONG (WINAPI*)(OSVERSIONINFOW*);
    auto getVersion = reinterpret_cast<GetVersion>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion"));
    OSVERSIONINFOW version = {}; version.dwOSVersionInfoSize = sizeof(version);
    if (getVersion && getVersion(&version) == 0)
        wc3::Log("Windows kernel=%lu.%lu build=%lu", version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber);
    if (!game) { wc3::LogError("Game.dll not loaded after startup wait"); return 0; }
    if (!wc3::Bind(game)) { wc3::LogError("Native API bind failed; no hooks installed"); return 0; }
    // Direct RoC launches must also opt out of Windows bitmap scaling before Warcraft creates its window.
    // Launcher startup supplies HIGHDPIAWARE; leave an already-created window's awareness unchanged.
    if (!IsProcessDPIAware() && !FindWindowA("Warcraft III", nullptr))
        wc3::Log("Early DPI awareness enabled=%d", SetProcessDPIAware());
    uintptr_t base = gameBase = reinterpret_cast<uintptr_t>(game);
    // Verify the render callback prologue before installing any game hook.
    const unsigned char expected[] = {0x53,0x56,0x8B,0xF1,0x8B,0x8E,0x38,0x03,0x00,0x00};
    if (memcmp(reinterpret_cast<void*>(base + 0x395900), expected, sizeof(expected))) {
        wc3::Log("World render signature mismatch: no hooks installed"); return 0;
    }
    controller.Configure(root, base); overlay.Configure(root);
    if (MH_Initialize() != MH_OK) { wc3::Log("MinHook initialization failed"); return 0; }
    // Map timers may request RTS camera presets repeatedly; live FPS owns those native camera writes.
    MapCameraGuard::Install(base, []() { return healthy && controller.BlocksMapCamera(); }, Hook);
    ActorRenderFilter::Install(base, Hook);
    NativeFloatingText::Install(base, Hook);
    FpsCombatGuard::Install(base, []() { return healthy ? controller.ViewActor() : 0; },
        [](uintptr_t unit) { return healthy && controller.BlocksPassiveUnit(unit); },
        [](uintptr_t attack) { return healthy && controller.BlocksPassiveAttack(attack); }, Hook);
    Hook(reinterpret_cast<void*>(base + 0x3BC4D0), reinterpret_cast<void*>(PauseHook), reinterpret_cast<void**>(&originalPause));
    Hook(reinterpret_cast<void*>(wc3::ShowInterface), reinterpret_cast<void*>(InterfaceHook), reinterpret_cast<void**>(&originalInterface));
    if (!Hook(reinterpret_cast<void*>(base + 0x395900), reinterpret_cast<void*>(WorldHook), reinterpret_cast<void**>(&originalWorld))) return 0;
    const unsigned char uiSignature[] = {0x83,0xEC,0x10,0x53,0x55,0x56,0x57,0x8B,0xF9};
    if (!memcmp(reinterpret_cast<void*>(base + 0x60C580), uiSignature, sizeof(uiSignature)))
        Hook(reinterpret_cast<void*>(base + 0x60C580), reinterpret_cast<void*>(UIHook), reinterpret_cast<void**>(&originalUI));
    else wc3::Log("UI signature mismatch; original UI retained");
    const unsigned char perspectiveSignature[] = {0x51, 0xD9, 0x44, 0x24, 0x0C};
    if (!memcmp(reinterpret_cast<void*>(base + 0x7B66F0), perspectiveSignature, sizeof(perspectiveSignature)))
        Hook(reinterpret_cast<void*>(base + 0x7B66F0), reinterpret_cast<void*>(PerspectiveHook), reinterpret_cast<void**>(&originalPerspective));
    else wc3::Log("Perspective signature mismatch; original near plane retained");
    // The verified Game.dll import uses the two-argument layer-buffer API.
    HMODULE gl = GetModuleHandleA("opengl32.dll");
    Hook(reinterpret_cast<void*>(GetProcAddress(gl, "glViewport")), reinterpret_cast<void*>(ViewportHook), reinterpret_cast<void**>(&originalViewport));
    Hook(reinterpret_cast<void*>(GetProcAddress(gl, "glScissor")), reinterpret_cast<void*>(ScissorHook), reinterpret_cast<void**>(&originalScissor));
    Hook(reinterpret_cast<void*>(GetProcAddress(gl, "wglDeleteContext")), reinterpret_cast<void*>(DeleteContextHook), reinterpret_cast<void**>(&originalDeleteContext));
    Hook(reinterpret_cast<void*>(GetProcAddress(gl, "glDrawElements")), reinterpret_cast<void*>(DrawElementsHook), reinterpret_cast<void**>(&originalDrawElements));
    Hook(reinterpret_cast<void*>(GetProcAddress(gl, "wglSwapLayerBuffers")), reinterpret_cast<void*>(SwapHook), reinterpret_cast<void**>(&originalSwap));
    wc3::Log("WarcraftCS initialized. Offline only. Select an owned unit and press F6.");
    return 0;
}
BOOL PluginRuntime::Attach(HINSTANCE module) {
        DisableThreadLibraryCalls(module); GetModuleFileNameA(module, root, MAX_PATH);
        // Miles probes then unloads .mix providers; hooks require the module to stay resident.
        HMODULE pinned = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCSTR>(PluginRuntime::Attach), &pinned);
        char* slash = strrchr(root, '\\'); if (slash) *slash = 0;
        strcat_s(root, "\\WarcraftCS");
        // Defer engine work until after the loader lock has been released.
        HANDLE thread = CreateThread(nullptr, 0, Initialize, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    return TRUE;
}
