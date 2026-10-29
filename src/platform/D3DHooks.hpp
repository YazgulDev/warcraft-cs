#pragma once
#include <windows.h>
#include <functional>

#include <d3d9.h>

// Platform adapter that routes the FPS HUD into Direct3D 9. Warcraft can render through
// OpenGL or Direct3D 9; this module hooks the Direct3DCreate9 export, the returned factory's
// CreateDevice vtable slot and the device Present/Reset slots, so the same overlay draws
// through the D3D canvas without touching the OpenGL path. Only one renderer is active per
// frame, so the GL present anchor and these hooks never race. Native D3D UI filtering/capture
// is owned separately by D3DNativeUi.
namespace D3DHooks {
// Called from the device Present hook before the real Present flips the back buffer.
using PresentCallback = std::function<void(IDirect3DDevice9*, HWND, int, int)>;
// Called for every projection transform the game sets, with the matrix (16 floats, row major)
// and the viewport that is active at that moment, so the caller can keep only the world pass.
using ProjectionCallback = void(*)(const float* matrix, const int* viewport);
// Claims the Direct3DCreate9 exports from a thread started in DllMain, i.e. long before Install's
// engine work runs. Warcraft builds its device during startup and an exclusive-fullscreen device
// can never be re-created next to it (every probe comes back D3DERR_DEVICELOST), so the engine's
// own CreateDevice call is the only chance to anchor a fullscreen session. Idempotent: Install
// re-uses whatever this already hooked.
void StartEarlyInstall();
// Loads d3d9 and hooks device creation. Safe on renders that never use D3D: the hook stays
// inert unless the game actually creates a Direct3D device.
bool Install(PresentCallback present, ProjectionCallback projection);
bool Installed();
// While the FPS view is live, native 2D chrome (classic console strip and top bar) is clipped
// to the given world region: batches drawn under an orthographic projection are scissored,
// every perspective (world) draw disables the scissor again. Direct3D has no GL submission
// pass. D3DNativeUi provides the complete native UI geometry filter; this retains viewport/band handling.
void SetUiSuppression(bool enabled, const RECT& worldRegion);
}
