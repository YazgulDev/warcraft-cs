#include "ReticleView.hpp"
#include "ReticleDiagnostics.hpp"
#include <windows.h>
#include <gl/GL.h>

namespace {
void BeginReticle(float red, float green, float blue, bool sampled) {
    // Warcraft can leave masks/stipple/wireframe behind; normalize the filled reticle and restore them afterwards.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);
    glDisable(GL_POLYGON_STIPPLE); glDisable(GL_COLOR_LOGIC_OP);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColor4f(red, green, blue, 1);
    ReticleDiagnostics::Prepared(sampled); glBegin(GL_QUADS);
}
void Rectangle(float left, float top, float right, float bottom) {
    glVertex2f(left, top); glVertex2f(right, top);
    glVertex2f(right, bottom); glVertex2f(left, bottom);
}
void EndReticle(bool sampled) {
    glEnd(); glPopAttrib(); ReticleDiagnostics::End(sampled);
}
}

void ReticleView::DrawHipFire(float width, float height, float recoil) {
    // Retain the original two-pixel arms and recoil gap at the true center of the FPS viewport.
    float x = width * .5f, y = height * .5f, gap = 5 + recoil * 4;
    bool sampled = ReticleDiagnostics::Begin("hip-fire", x, y, gap, 2, recoil, 4);
    BeginReticle(.5f, 1, .25f, sampled);
    Rectangle(x - gap - 10, y - 1, x - gap, y + 1);
    Rectangle(x + gap, y - 1, x + gap + 10, y + 1);
    Rectangle(x - 1, y - gap - 10, x + 1, y - gap);
    Rectangle(x - 1, y + gap, x + 1, y + gap + 10);
    EndReticle(sampled);
}

void ReticleView::DrawScope(float cx, float cy, float radius, float thickness) {
    // The AWP keeps its black center hairs and resolution-scaled thickness using the same reliable primitive.
    float half = thickness * .5f;
    bool sampled = ReticleDiagnostics::Begin("scope", cx, cy, radius, thickness, 0, 2);
    BeginReticle(0, 0, 0, sampled);
    Rectangle(cx - radius, cy - half, cx + radius, cy + half);
    Rectangle(cx - half, cy - radius, cx + half, cy + radius);
    EndReticle(sampled);
}
