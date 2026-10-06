#include "Overlay.hpp"
#include "ScopeView.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

static GLuint BitmapFont(HDC dc, int size) {
    // Rasterize each size directly so enlarged status text stays sharp instead of stretching pixels.
    HFONT font = CreateFontA(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, "Consolas");
    HGDIOBJ old = SelectObject(dc, font); GLuint lists = glGenLists(96);
    wglUseFontBitmapsA(dc, 32, 96, lists); SelectObject(dc, old); DeleteObject(font);
    return lists;
}
void Overlay::Font(HDC dc, int statusSize) {
    if (!font_) font_ = BitmapFont(dc, 23);
    // Only the bottom HUD scales with resolution; rebuild its glyphs when the viewport size changes.
    if (statusFontSize_ != statusSize) {
        if (statusFont_) glDeleteLists(statusFont_, 96);
        statusFont_ = BitmapFont(dc, statusSize); statusFontSize_ = statusSize;
    }
}
void Overlay::Text(float x, float y, const char* text, GLuint font) {
    glRasterPos2f(x, y); glListBase((font ? font : font_) - 32);
    glCallLists(GLsizei(strlen(text)), GL_UNSIGNED_BYTE, text);
}
void Overlay::ResetGraphics(bool deleteObjects) {
    // GPU names belong to one context; rebuild both skins and glyphs after replacement or focus recovery.
    for (int i = 0; i < WeaponSlots::Count; ++i) { guns_[i].Reset(deleteObjects); loaded_[i] = attempted_[i] = false; }
    if (deleteObjects) {
        if (font_) glDeleteLists(font_, 96);
        if (statusFont_) glDeleteLists(statusFont_, 96);
    }
    font_ = statusFont_ = 0; statusFontSize_ = 0;
}
void Overlay::ContextDeleted(HGLRC context) {
    if (context_ != context) return;
    ResetGraphics(false); context_ = nullptr;
    wc3::Log("Overlay context deleted; GPU names forgotten");
}
void Overlay::Draw(HDC dc, const ShooterController& controller) {
    if (!controller.Visible() || !wglGetCurrentContext()) return;
    HGLRC current = wglGetCurrentContext();
    if (context_ != current) {
        ResetGraphics(false); context_ = current;
        wc3::Log("Overlay context acquired: %p", current);
    } else if (refreshPending_) {
        ResetGraphics(true); wc3::Log("Overlay rebuilt after focus return");
    }
    refreshPending_ = false;
    // Preserve every GL state touched so Warcraft's next frame renders normally.
    GLint oldMode; glGetIntegerv(GL_MATRIX_MODE, &oldMode); glPushAttrib(GL_ALL_ATTRIB_BITS);
    HWND window = WindowFromDC(dc); RECT client; GetClientRect(window, &client);
    int width = client.right, height = client.bottom;
    if (height <= 0 || width <= 0) { glPopAttrib(); return; }
    glViewport(0, 0, width, height);
    // Warcraft uses multiple texture units; isolate the overlay on unit zero.
    using ActiveTexture = void (APIENTRY*)(GLenum);
    auto activeTexture = reinterpret_cast<ActiveTexture>(wglGetProcAddress("glActiveTextureARB"));
    if (!activeTexture) activeTexture = reinterpret_cast<ActiveTexture>(wglGetProcAddress("glActiveTexture"));
    GLint oldTexture = 0x84C0, textureUnits = 1;
    if (activeTexture) {
        glGetIntegerv(0x84E0, &oldTexture); glGetIntegerv(0x84E2, &textureUnits);
        for (int unit = 0; unit < textureUnits; ++unit) {
            activeTexture(0x84C0 + unit);
            glDisable(GL_TEXTURE_1D); glDisable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T);
        }
        activeTexture(0x84C0);
    }
    glMatrixMode(GL_TEXTURE); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_LIGHTING); glDisable(GL_FOG);
    glDisable(GL_SCISSOR_TEST); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.1f);
    // Warcraft leaves stencil/clip state active; HUD primitives must not inherit its world masks.
    glDisable(GL_STENCIL_TEST);
    for (int plane = 0; plane < 6; ++plane) glDisable(GL_CLIP_PLANE0 + plane);
    glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    int index = controller.WeaponIndex();
    // Bring rifles closer while their original pose keeps forearms entering through the lower edge.
    static constexpr float projectionScales[] = {0.72f, 0.78f, 1.0f, 0.78f, 1.0f, 0.95f, 0.85f};
    float projectionScale = projectionScales[index];
    float top = std::tan(45.0f * 0.01745329252f) * projectionScale; float side = top * width / height;
    glFrustum(-side, side, -top, top, 1, 200);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    if (!attempted_[index]) {
        attempted_[index] = true;
        static const char* names[] = {"ak47", "m4a1", "usp", "awp", "knife", "c4", "sword"};
        loaded_[index] = guns_[index].Load(root_ + "\\assets\\" + names[index] + ".wcg");
    }
    if (loaded_[index] && !controller.Status().contained && !controller.Status().hidden && !controller.Scoped() && (index != WeaponSlots::C4 || controller.Ammo() > 0)) {
        // Give the weapon its own depth pass: back faces must not paint over the skin.
        glDepthMask(GL_TRUE); glClearDepth(1); glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthRange(0, 1);
        // Preserve each model's original arm origin instead of exposing detached elbow ends.
        guns_[index].Draw(controller.Animation(), controller.AnimationTime());
        glDisable(GL_DEPTH_TEST);
    }
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glDisable(GL_TEXTURE_2D);
    // A swallowed/transported unit has no world viewpoint: hide equipment and retain the health/status HUD.
    bool unavailableView = controller.Status().contained || controller.Status().hidden;
    if (unavailableView) {
        glColor4f(0.015f, 0.015f, 0.015f, 1);
        glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(float(width), 0);
        glVertex2f(float(width), float(height)); glVertex2f(0, float(height)); glEnd();
        glColor4f(1, .8f, .2f, 1);
        Text(25, float(height) * .5f, controller.Status().devoured ?
            "SWALLOWED: DIGESTING | F6: RETURN TO RTS TO RESCUE THIS UNIT" : "UNIT IS OUTSIDE THE WORLD | F6: RETURN TO RTS");
    }
    else if (controller.Scoped()) ScopeView::Draw(float(width), float(height));
    else {
        glColor4f(0.5f, 1.0f, 0.25f, 1); glLineWidth(2);
        // The FPS world frame fills the window, so the camera ray intersects its true center.
        float x = width * 0.5f, y = height * 0.5f, gap = 5 + controller.Recoil() * 4;
        glBegin(GL_LINES);
        glVertex2f(x - gap - 10, y); glVertex2f(x - gap, y); glVertex2f(x + gap, y); glVertex2f(x + gap + 10, y);
        glVertex2f(x, y - gap - 10); glVertex2f(x, y - gap); glVertex2f(x, y + gap); glVertex2f(x, y + gap + 10);
        glEnd();
    }
    // Scale the bottom HUD from a readable 42-pixel font at 1080p, with safe edge padding.
    int statusSize = std::clamp(int(std::lround(height * (42.0f / 1080.0f))), 26, 84);
    Font(dc, statusSize);
    controller.Hits().Draw(float(width), float(height));
    char status[160]; sprintf_s(status, "HP %.0f   %s   %d / %d", controller.Health(),
        ShooterController::weapons[index].name, controller.Ammo(), controller.Reserve());
    // Melee equipment has no magazine; show only its name instead of a misleading empty-ammo counter.
    if (WeaponSlots::Melee(index)) sprintf_s(status, "HP %.0f   %s", controller.Health(), ShooterController::weapons[index].name);
    float statusPad = statusSize * 0.75f, statusY = height - statusPad;
    glColor4f(1.0f, 0.85f, 0.3f, 1); Text(statusPad, statusY, status, statusFont_);
    // Installation progress and fuse are independent of which weapon is currently equipped.
    if (controller.PlantProgress() > 0) {
        char message[80]; sprintf_s(message, "PLANTING C4  %.0f%%", controller.PlantProgress() / 3 * 100);
        Text(width * 0.5f - 180, height * 0.65f, message, statusFont_);
    }
    if (controller.BombRemaining() >= 0) {
        char message[80]; sprintf_s(message, "C4  %.0f SEC", double(std::ceil(controller.BombRemaining())));
        Text(float(width - statusSize * 9), statusY, message, statusFont_);
    }
    Text(25, 59, "1-5: GUNS / KNIFE | 6: C4 (HOLD FIRE) | 7: GREATSWORD | MELEE RMB: THRUST | E: ITEM | F8: CONFIG");
    // The squad mode/count remains visible after its short confirmation disappears.
    // Show the independent release key beside both recruitment policies.
    char squad[128];sprintf_s(squad,"H: FIGHT | O: FOLLOW | J: RELEASE | SQUAD %u %s",unsigned(controller.SquadCount()),controller.SquadCount() ? (controller.SquadPassive() ? "FOLLOW" : "COMBAT") : "");
    Text(25,88,squad);
    // Keep the creator credit visible alongside the controls.
    Text(25, 30, "Warcraft CS by Yazgul | F6: RTS | F7: AMMO | WASD | SPACE: JUMP | CTRL: DUCK | R: RELOAD");
    if (controller.RefillNotice()) Text(statusPad, statusY - statusSize - 16, controller.AmmoMessage(), statusFont_);
    // Ground items advertise interaction without issuing a walk-to-item RTS order.
    if (controller.ItemNearby()) Text(width*.5f-120,height*.72f,"E: PICK UP ITEM",statusFont_);
    // Explain blocked controls next to the status HUD while the corresponding Warcraft effect lasts.
    if (*controller.Status().Label()) {
        char effect[96]; sprintf_s(effect, "%s", controller.Status().Label());
        if (!controller.Status().immobilized && std::abs(controller.Status().speedScale - 1) >= 0.01f)
            sprintf_s(effect, "%s  %.0f%% SPEED", controller.Status().Label(), controller.Status().speedScale * 100);
        Text(statusPad, statusY - (statusSize + 16) * (controller.RefillNotice() ? 2 : 1), effect, statusFont_);
    }
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_TEXTURE); glPopMatrix();
    glMatrixMode(oldMode); glPopAttrib();
    if (activeTexture) activeTexture(oldTexture);
}
