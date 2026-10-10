#pragma once

// Both aiming modes use filled geometry instead of driver-dependent OpenGL line rasterization.
class ReticleView {
public:
    static void DrawHipFire(float width, float height, float recoil);
    static void DrawScope(float cx, float cy, float radius, float thickness);
};
