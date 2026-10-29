#pragma once
#include <d3d9.h>
#include <cstdint>

// Own the native UI submission filter and portrait render target, separately from the FPS canvas.
namespace D3DNativeUi {
void Attach(IDirect3DDevice9* device);
void Configure(bool enabled,uint32_t actor,bool hideCursor=true);
void Phase(bool nativeUi);
void Overlay(bool active);
void Projection(IDirect3DDevice9* device,bool orthographic);
void Viewport(IDirect3DDevice9* device,D3DVIEWPORT9& viewport,int width,int height);
void Finish();
void BeforeReset(IDirect3DDevice9* device);
void DrawPortrait(int x,int y,int side);
}
