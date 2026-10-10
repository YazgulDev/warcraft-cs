#include "ScopeView.hpp"
#include "ReticleView.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <algorithm>
#include <cmath>

void ScopeView::Draw(float width, float height) {
    float cx = width * 0.5f, cy = height * 0.5f;
    float radius = std::min(width, height) * 0.46f, outside = std::hypot(width, height);
    // An opaque annulus removes peripheral vision, including widescreen side regions.
    glColor4f(0, 0, 0, 1); glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 256; ++i) {
        float angle = i * 6.28318530718f / 256, x = std::cos(angle), y = std::sin(angle);
        glVertex2f(cx + x * radius, cy + y * radius);
        glVertex2f(cx + x * outside, cy + y * outside);
    }
    glEnd();
    // Scope hairs share the filled reticle path so inherited line stipple cannot hide aiming marks.
    ReticleView::DrawScope(cx, cy, radius, std::max(1.0f, height / 720.0f));
}
