#pragma once
#include <d3d9.h>
#include "../presentation/MinimapLayout.hpp"

// Capture native terrain/fog/markers on the GPU, keeping every hidden console draw away from the world.
namespace D3DMinimap {
void Attach(IDirect3DDevice9* device);
void Configure(const RECT& nativeBounds,int width,int height,bool enabled);
bool BeginNativeUi(IDirect3DDevice9* device);
bool Capturing(IDirect3DDevice9* device);
void FinishFrame();
void BeginWorld();
void BeforeReset(IDirect3DDevice9* device);
void Draw(const MinimapLayout& layout);
}
