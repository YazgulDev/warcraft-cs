#pragma once

// Presentation diagnostics observe actual GL submission without reading pixels or owning aiming rules.
class ReticleDiagnostics {
public:
    static bool Begin(const char* mode, float cx, float cy, float extent, float thickness, float recoil, unsigned quads);
    static void Prepared(bool sampled);
    static void End(bool sampled);
    static void Skip(const char* reason, int width = 0, int height = 0, unsigned long worldAgeMs = 0);
};
