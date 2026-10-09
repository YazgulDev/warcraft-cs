#include "HitFeedback.hpp"
#include <gl/GL.h>
#include <algorithm>
#include <cmath>

void HitFeedback::Record() { tick_ = GetTickCount(); active_ = true; }
void HitFeedback::Clear() { active_ = false; }
void HitFeedback::Draw(float width, float height) const {
    float age = (GetTickCount() - tick_) * 0.001f;
    if (!active_ || age >= 0.65f) return;
    float scale = std::clamp(height / 548.0f, 0.6f, 2.5f), cx = width * 0.5f, cy = height * 0.5f;
    // Only four fading crosshair strokes confirm damage, regardless of target classification.
    glLineWidth(3 * scale); glColor4f(0.95f, 0.95f, 0.95f, 1 - age / 0.65f);
    glBegin(GL_LINES);
    for (int i = 0; i < 4; ++i) {
        float angle = 0.78539816f + i * 1.57079633f;
        glVertex2f(cx + std::cos(angle) * 13 * scale, cy + std::sin(angle) * 13 * scale);
        glVertex2f(cx + std::cos(angle) * 22 * scale, cy + std::sin(angle) * 22 * scale);
    }
    glEnd();
}
