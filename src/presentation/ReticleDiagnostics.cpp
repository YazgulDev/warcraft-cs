#include "ReticleDiagnostics.hpp"
#include "../platform/DiagnosticLog.hpp"
#include <gl/GL.h>
#include <cstring>

namespace {
DWORD previous = 0;
const char* previousState = "";
HGLRC previousContext = nullptr;
unsigned frames = 0;

void Trace(const char* format, ...) {
    va_list args; va_start(args, format); DiagnosticLog::Write("TRACE", format, args); va_end(args);
}
void Error(const char* format, ...) {
    va_list args; va_start(args, format); DiagnosticLog::Write("ERROR", format, args); va_end(args);
}
bool Sample(const char* state) {
    ++frames;
    HGLRC context = wglGetCurrentContext();
    bool changed = strcmp(previousState, state) != 0 || previousContext != context;
    // State/context transitions bypass the interval but still obey Detailed=false.
    bool sampled = DiagnosticLog::Due(previous, changed);
    previousState = state; previousContext = context;
    return sampled;
}
void State(const char* phase) {
    GLint viewport[4] = {}, scissor[4] = {}, polygon[2] = {}, blendSrc = 0, blendDst = 0;
    GLboolean mask[4] = {}; GLfloat color[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport); glGetIntegerv(GL_SCISSOR_BOX, scissor);
    glGetIntegerv(GL_POLYGON_MODE, polygon); glGetBooleanv(GL_COLOR_WRITEMASK, mask);
    glGetFloatv(GL_CURRENT_COLOR, color);
    glGetIntegerv(GL_BLEND_SRC, &blendSrc); glGetIntegerv(GL_BLEND_DST, &blendDst);
    Trace("reticle state phase=%s viewport=%d,%d,%d,%d scissor=%d:%d,%d,%d,%d colorMask=%d%d%d%d color=%.2f,%.2f,%.2f,%.2f polygon=%04X,%04X polygonStipple=%d lineStipple=%d logicOp=%d depth=%d stencil=%d alpha=%d blend=%d:%04X,%04X cull=%d texture2D=%d",
        phase, viewport[0], viewport[1], viewport[2], viewport[3], glIsEnabled(GL_SCISSOR_TEST),
        scissor[0], scissor[1], scissor[2], scissor[3], mask[0], mask[1], mask[2], mask[3],
        color[0], color[1], color[2], color[3], polygon[0], polygon[1], glIsEnabled(GL_POLYGON_STIPPLE),
        glIsEnabled(GL_LINE_STIPPLE), glIsEnabled(GL_COLOR_LOGIC_OP), glIsEnabled(GL_DEPTH_TEST),
        glIsEnabled(GL_STENCIL_TEST), glIsEnabled(GL_ALPHA_TEST), glIsEnabled(GL_BLEND), blendSrc, blendDst,
        glIsEnabled(GL_CULL_FACE), glIsEnabled(GL_TEXTURE_2D));
}
unsigned Errors(const char* phase) {
    unsigned count = 0;
    // Empty the sampled error queue before submission so host errors are labeled separately.
    // The bound prevents a broken driver from hanging the game's render callback.
    for (; count < 16; ++count) {
        GLenum error = glGetError(); if (error == GL_NO_ERROR) break;
        Error("reticle OpenGL error phase=%s code=%04X", phase, error);
    }
    if (count == 16) Error("reticle OpenGL error queue limit reached phase=%s; subsequent errors may be inherited", phase);
    return count;
}
}

bool ReticleDiagnostics::Begin(const char* mode, float cx, float cy, float extent, float thickness, float recoil, unsigned quads) {
    if (!Sample(mode)) return false;
    Trace("reticle draw begin mode=%s context=%p frames=%u center=%.2f,%.2f %s=%.2f thickness=%.2f recoil=%.2f primitive=GL_QUADS quads=%u vertices=%u",
        mode, wglGetCurrentContext(), frames, cx, cy, strcmp(mode, "scope") == 0 ? "radius" : "gap",
        extent, thickness, recoil, quads, quads * 4);
    frames = 0;
    Errors("before-draw-host"); State("inherited");
    return true;
}
void ReticleDiagnostics::Prepared(bool sampled) {
    // Queries must precede glBegin: querying state inside immediate-mode geometry is itself invalid.
    if (sampled) State("prepared");
}
void ReticleDiagnostics::End(bool sampled) {
    if (!sampled) return;
    unsigned errors = Errors("draw-and-restore");
    Trace("reticle draw end context=%p observedErrors=%u submission=completed", wglGetCurrentContext(), errors);
}
void ReticleDiagnostics::Skip(const char* reason, int width, int height, unsigned long worldAgeMs) {
    // No GL queries are needed when there is no context or FPS rendering is deliberately suspended.
    if (!Sample(reason)) return;
    Trace("reticle skipped reason=%s context=%p frames=%u size=%dx%d worldAgeMs=%lu",
        reason, wglGetCurrentContext(), frames, width, height, worldAgeMs);
    frames = 0;
}
