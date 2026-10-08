#include "WarcraftApi.hpp"
#include "ShooterController.hpp"
#include "Overlay.hpp"
#include "MapCameraGuard.hpp"
#include "ActorRenderFilter.hpp"
#include "FpsCombatGuard.hpp"
#include <MinHook.h>
#include <cstring>
#ifdef WCS_STATUS_TEST
#include "../tests/StatusEffectScene.hpp"
#endif
#ifdef WCS_GAMEPLAY_TEST
#include "../tests/RuneCombatScene.hpp"
#endif
#ifdef WCS_TREE_WHEEL_TEST
#include "../tests/TreeAndWheelScene.hpp"
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

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM key, LPARAM data) {
    if (message==WM_INPUT) {
        // Raw packets retain fast movement and keep 360-degree look independent of cursor recentering.
        controller.MouseInput(data,healthy && controller.Visible() && !controller.Buying() && GetForegroundWindow()==window);
        if (healthy && controller.Visible()) return DefWindowProcA(window,message,key,data);
    }
    // Returning from Alt-Tab may replace the GL context or its resources; refresh on the render thread.
    if (message == WM_ACTIVATEAPP) {
        controller.ResetMouse();
        if (key) overlay.RefreshAfterFocus();
    }
    // F6 belongs to the extension even before FPS is active, avoiding native quicksave.
    if (key == VK_F6 && (message == WM_KEYDOWN || message == WM_KEYUP)) {
        if (message == WM_KEYDOWN && !(data & (1L << 30))) controller.RequestToggle();
        return 0;
    }
    // Consume FPS inputs so Warcraft does not also issue RTS orders or select units.
    if (healthy && controller.Visible()) {
        // B/period and menu rows are mailbox events; no native economy calls occur in the input handler.
        if ((message==WM_KEYDOWN || message==WM_KEYUP) && (key=='B' || key==VK_OEM_PERIOD ||
            (controller.Buying() && key>='0' && key<='9'))) {
            if (message==WM_KEYDOWN && !(data&(1L<<30))) {
                if (key=='B') controller.RequestBuyToggle();
                else if (key==VK_OEM_PERIOD) controller.RequestBuyAmmo();
                else controller.RequestBuyKey(int(key-'0'));
            }
            return 0;
        }
        if (controller.Buying() && message==WM_LBUTTONDOWN) {
            RECT client={};GetClientRect(window,&client);
            controller.RequestBuyClick(short(LOWORD(data)),short(HIWORD(data)),client.right,client.bottom);return 0;
        }
        // Queue each signed wheel packet for the game tick; Warcraft must not also zoom its RTS camera.
        if (message == WM_MOUSEWHEEL) {
            if (GetForegroundWindow() == window) controller.RequestWeaponWheel(GET_WHEEL_DELTA_WPARAM(key));
            return 0;
        }
        // Consume F7 before Warcraft can interpret it as a strategy shortcut.
        if (key == VK_F7 && (message == WM_KEYDOWN || message == WM_KEYUP)) {
            if (message == WM_KEYDOWN && !(data & (1L << 30))) controller.RequestRefill();
            return 0;
        }
        // Preserve short pickup/squad/config taps; J releases followers without leaving FPS.
        if ((key=='E' || key=='H' || key=='O' || key=='J' || key==VK_F8) && (message==WM_KEYDOWN || message==WM_KEYUP)) {
            if (message==WM_KEYDOWN && !(data&(1L<<30))) {
                if (key=='E') controller.RequestItemPickup();
                else if (key=='H' || key=='O') controller.RequestSquad(key=='O');
                else if (key=='J') controller.RequestSquadRelease();
                else controller.RequestSettingsReload();
            }
            return 0;
        }
        // The native pause menu requires restored RTS input before handling its hotkey.
        // Windows delivers F10 as a system key even when Alt is not pressed.
        if (key == VK_F10 && (message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)) {
            if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && !(data & (1L << 30))) controller.RequestMenu();
            return 0;
        }
        if ((message == WM_KEYDOWN || message == WM_KEYUP) &&
            (key == 'W' || key == 'A' || key == 'S' || key == 'D' || key == 'R' ||
             key == VK_F6 || key == VK_SPACE || key == VK_CONTROL || key == VK_SHIFT || (key >= '1' && key <= '1' + WeaponSlots::Count - 1))) return 0;
        if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONDOWN ||
            message == WM_RBUTTONUP || message == WM_MOUSEMOVE) return 0;
    }
    return CallWindowProcA(originalWindow, window, message, key, data);
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
        healthy = false; wc3::Log("Controller fault %08X; disabled", GetExceptionCode());
        // A diagnostic/native failure must never keep FPS input or camera ownership locked over an exit dialog.
        __try { controller.RecoverFault(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { wc3::Log("Native view restoration failed; input remains released"); }
    }
}
static int __fastcall WorldHook(uintptr_t ui, uintptr_t unused) {
    DWORD now = GetTickCount();
    lastWorld = now;
    // RenderWorld's this pointer is not CGameUI; query the real UI for cinematic flags.
    uintptr_t gameUI = CurrentUI(); ObserveMap(gameUI);
    if (healthy && gameUI) SafeTick(gameUI);
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
    // Keep layout evaluation running while hiding its final graphics after the world pass.
    nativeUIPhase = false;
    // Suppress only the controlled actor during world drawing; menus, portraits and RTS still render it.
    ActorRenderFilter::Begin(healthy ? controller.ViewActor() : 0);
    int result = originalWorld(ui, unused);
    ActorRenderFilter::End();
    nativeUIPhase = true;
    return result;
}
static void __cdecl PauseHook(BOOL value) { controller.SetPaused(value != FALSE); originalPause(value); }
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
    // The RTS near plane clips nearby terrain at eye level; FPS needs a short near plane.
    if (controller.Visible() && GetTickCount() - lastWorld < 500) nearZ = std::min(nearZ, 8.0f);
    originalPerspective(output, unused, fov, aspect, nearZ, farZ);
}
static BOOL WINAPI SwapHook(HDC dc, UINT planes) {
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
    overlayPass = false; nativeUIPhase = false;
    return originalSwap(dc, planes);
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
    wc3::OpenLog(root);
    if (!game || !wc3::Bind(game)) return 0;
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
    Hook(reinterpret_cast<void*>(GetProcAddress(gl, "wglDeleteContext")), reinterpret_cast<void*>(DeleteContextHook), reinterpret_cast<void**>(&originalDeleteContext));
    Hook(reinterpret_cast<void*>(GetProcAddress(gl, "glDrawElements")), reinterpret_cast<void*>(DrawElementsHook), reinterpret_cast<void**>(&originalDrawElements));
    Hook(reinterpret_cast<void*>(GetProcAddress(gl, "wglSwapLayerBuffers")), reinterpret_cast<void*>(SwapHook), reinterpret_cast<void**>(&originalSwap));
    wc3::Log("WarcraftCS initialized. Offline only. Select an owned unit and press F6.");
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module); GetModuleFileNameA(module, root, MAX_PATH);
        // Miles probes then unloads .mix providers; hooks require the module to stay resident.
        HMODULE pinned = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCSTR>(DllMain), &pinned);
        char* slash = strrchr(root, '\\'); if (slash) *slash = 0;
        strcat_s(root, "\\WarcraftCS");
        // Defer engine work until after the loader lock has been released.
        HANDLE thread = CreateThread(nullptr, 0, Initialize, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
