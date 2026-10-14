#include "Overlay.hpp"
#include "ScopeView.hpp"
#include "ReticleView.hpp"
#include "TargetMarkerView.hpp"
#include "ReticleDiagnostics.hpp"
#include "BuyMenuView.hpp"
#include "HudHints.hpp"
#include "../platform/NativeMinimap.hpp"
#include "../platform/DiagnosticLog.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

static GLuint BitmapFont(HDC dc, int size, int* glyphWidth=nullptr) {
    // Rasterize each size directly so enlarged status text stays sharp instead of stretching pixels.
    HFONT font = CreateFontA(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, "Consolas");
    HGDIOBJ old = SelectObject(dc, font); GLuint lists = glGenLists(96);
    // Use the actual monospace advance so wrapping remains correct at different Windows DPI settings.
    if (glyphWidth) {TEXTMETRICA metrics={};GetTextMetricsA(dc,&metrics);*glyphWidth=std::max(1,int(metrics.tmAveCharWidth));}
    wglUseFontBitmapsA(dc, 32, 96, lists); SelectObject(dc, old); DeleteObject(font);
    return lists;
}
void Overlay::Font(HDC dc, int statusSize) {
    if (!font_) font_ = BitmapFont(dc, 20, &hintGlyphWidth_);
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
    spellHud_.Reset(deleteObjects);
    sky_.Reset(deleteObjects);
    // GPU names belong to one context; never delete old numeric IDs in a replacement context.
    for (int i = 0; i < WeaponSlots::Count; ++i) { guns_[i].Reset(deleteObjects); loaded_[i] = attempted_[i] = false; }
    // Menu models obey the same context ownership rules as first-person weapons.
    for (int i=0;i<WeaponSlots::Count;++i) { buyPreviews_[i].Reset(deleteObjects);previewLoaded_[i]=previewAttempted_[i]=false; }
    if (deleteObjects) {
        if (font_) glDeleteLists(font_, 96);
        if (statusFont_) glDeleteLists(statusFont_, 96);
        if (spellFont_) glDeleteLists(spellFont_,96);
    }
    font_ = statusFont_ = 0; statusFontSize_ = 0;
    spellFont_=0;spellFontSize_=0;
}
void Overlay::ContextDeleted(HGLRC context) {
    if (context_ != context) return;
    ResetGraphics(false); context_ = nullptr;
    wc3::Log("Overlay context deleted; GPU names forgotten");
}
void Overlay::Draw(HDC dc, const ShooterController& controller,NativeMinimap& minimap,const std::vector<UnitHealthBar>& bars,
    const WorldLabelView& barView,float nearPlane,float farPlane) {
    // Explain early returns as well as submitted geometry when investigating a missing aiming mark.
    if (!controller.Visible()) { ReticleDiagnostics::Skip("fps-inactive"); return; }
    // Unit view contains no mod HUD, arms, reticle, menu or hit feedback; the native sky remains in the world.
    if(controller.UnitFirstPerson())return;
    if (!wglGetCurrentContext()) { ReticleDiagnostics::Skip("no-gl-context"); return; }
    HGLRC current = wglGetCurrentContext();
    if (context_ != current) {
        ResetGraphics(false); context_ = current;
        wc3::Log("Overlay context acquired: %p", current);
        // Record the actual driver renderer for reports where the overlay initializes but primitives disappear.
        wc3::Log("Overlay GL vendor=%s renderer=%s version=%s lineStipple=%d",
            glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION), glIsEnabled(GL_LINE_STIPPLE));
    } else if (refreshPending_) {
        // Keep surviving resources: blind deletion after Alt-Tab can touch IDs recycled by the game.
        bool valid=(!font_ || glIsList(font_)) && (!statusFont_ || glIsList(statusFont_)) && (!spellFont_ || glIsList(spellFont_));
        valid=valid && sky_.Valid() && spellHud_.Valid();
        for (int i=0;i<WeaponSlots::Count;++i) if (loaded_[i] && !guns_[i].TexturesValid()) valid=false;
        for (int i=0;i<WeaponSlots::Count;++i) if (previewLoaded_[i] && !buyPreviews_[i].TexturesValid()) valid=false;
        if (!valid) { ResetGraphics(false); wc3::Log("Overlay lost resources forgotten after focus return"); }
        else wc3::Log("Overlay resources retained after focus return");
    }
    refreshPending_ = false;
    // Imports can keep the same path across maps; reload those textures in the active rendering context.
    if(spellIconsRefresh_) {spellHud_.Reset(true);spellIconsRefresh_=false;}
    // Preserve every GL state touched so Warcraft's next frame renders normally.
    GLint oldMode; glGetIntegerv(GL_MATRIX_MODE, &oldMode); glPushAttrib(GL_ALL_ATTRIB_BITS);
    HWND window = WindowFromDC(dc); RECT client = {};
    if (!window || !GetClientRect(window, &client)) {
        ReticleDiagnostics::Skip("client-rect-unavailable"); glPopAttrib(); return;
    }
    int width = client.right, height = client.bottom;
    if (height <= 0 || width <= 0) {
        ReticleDiagnostics::Skip("empty-client-rect", width, height); glPopAttrib(); return;
    }
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
    // Sky touches only far-depth pixels before the weapon's private depth clear, preserving native scenery.
    if (controller.Settings().csSky && !controller.Status().contained && !controller.Status().hidden)
        sky_.Draw(root_,controller.SkyName(),controller.ViewYaw(),controller.ViewPitch(),controller.ViewFov(),float(width)/height);
    // World depth still exists here; the first-person weapon clears it in the next pass.
    unsigned barCount=0;
    if(!controller.Status().contained && !controller.Status().hidden && !controller.Buying())
        barCount=HealthBarRenderer::Draw(bars,barView,width,height,nearPlane,farPlane);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    int index = controller.WeaponIndex();
    // Slightly tighten USP/AWP framing while preserving the original lower-edge arm origins.
    static constexpr float projectionScales[] = {0.72f, 0.78f, 0.94f, 0.73f, 1.0f, 0.95f, 0.85f};
    float projectionScale = projectionScales[index];
    float top = std::tan(45.0f * 0.01745329252f) * projectionScale; float side = top * width / height;
    glFrustum(-side, side, -top, top, 1, 200);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    if (!attempted_[index]) {
        attempted_[index] = true;
        static const char* names[] = {"ak47", "m4a1", "usp", "awp", "knife", "c4", "sword"};
        loaded_[index] = guns_[index].Load(root_ + "\\assets\\" + names[index] + ".wcg");
    }
    if (!controller.ThirdPerson() && loaded_[index] && !controller.Status().contained && !controller.Status().hidden && !controller.Scoped() && (index != WeaponSlots::C4 || controller.Ammo() > 0)) {
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
        ReticleDiagnostics::Skip(controller.Status().contained ? "unit-contained" : "unit-hidden", width, height);
        glColor4f(0.015f, 0.015f, 0.015f, 1);
        glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(float(width), 0);
        glVertex2f(float(width), float(height)); glVertex2f(0, float(height)); glEnd();
        glColor4f(1, .8f, .2f, 1);
        Text(25, float(height) * .5f, controller.Status().devoured ?
            "SWALLOWED: DIGESTING | F6: RETURN TO RTS TO RESCUE THIS UNIT" : "UNIT IS OUTSIDE THE WORLD | F6: RETURN TO RTS");
    }
    else if (controller.Scoped()) ScopeView::Draw(float(width), float(height));
    else ReticleView::DrawHipFire(float(width), float(height), controller.Recoil());
    // The circle and brackets identify the next reachable contact without a text caption.
    if(!unavailableView && !controller.Buying() && controller.ThirdPerson() && controller.Settings().thirdPerson.showTargetIndicator)
        TargetMarkerView::Draw(controller.AimTarget(),barView,width,height,nearPlane,farPlane);
    // Scale the bottom HUD from a readable 42-pixel font at 1080p, with safe edge padding.
    int statusSize = std::clamp(int(std::lround(height * (42.0f / 1080.0f))), 26, 84);
    Font(dc, statusSize);
    controller.Hits().Draw(float(width), float(height));
    char status[160]; sprintf_s(status, "HP %.0f   %s   %d / %d   GOLD %d", controller.Health(),
        ShooterController::weapons[index].name, controller.Ammo(), controller.Reserve(),controller.Gold());
    // Melee equipment has no magazine; show only its name instead of a misleading empty-ammo counter.
    if (WeaponSlots::Melee(index)) sprintf_s(status, "HP %.0f   %s   GOLD %d", controller.Health(), ShooterController::weapons[index].name,controller.Gold());
    // Hero combat has no magazine or CS weapon label; expose the mode and live native attack instead.
    if(controller.ThirdPerson())sprintf_s(status,"HP %.0f   UNIT ATTACK %.0f   GOLD %d",controller.Health(),
        controller.Status().attackDamage*controller.Settings().thirdPerson.damageMultiplier,controller.Gold());
    float statusPad = statusSize * 0.75f, statusY = height - statusPad;
    glColor4f(1.0f, 0.85f, 0.3f, 1); Text(statusPad, statusY, status, statusFont_);
    // Installation progress and fuse are independent of which weapon is currently equipped.
    if (controller.PlantProgress() > 0) {
        char message[80]; sprintf_s(message, "PLANTING C4  %.0f%%", controller.PlantProgress() / 3 * 100);
        Text(width * 0.5f - 180, height * 0.65f, message, statusFont_);
    }
    if (controller.BombRemaining() >= 0) {
        char message[80]; sprintf_s(message, "C4 x%u  %.0fs",unsigned(controller.BombCount()),double(std::ceil(controller.BombRemaining())));
        Text(float(width - statusSize * 9), statusY, message, statusFont_);
    }
    // Compact hints reflect the configured keys and wrap before the native minimap.
    auto key=[&](ShortcutAction action) {return controller.Shortcut(action);};
    auto mapLayout=MinimapLayout::Calculate(width,height,controller.Settings().minimapSizePercent);
    if(controller.MinimapVisible())minimap.Draw(mapLayout);
    std::string controls="Warcraft CS by Yazgul | "+key(ShortcutAction::ToggleFPS)+": RTS | "+key(ShortcutAction::ReloadSettings)+": CONFIG | "+key(ShortcutAction::Minimap)+": MINIMAP | "+key(ShortcutAction::Speed)+": SPEED | "+key(ShortcutAction::Fog)+": FOG";
    // The dedicated key exposes direct entry/RTS exit independently of the active view switch.
    controls+=" | "+key(ShortcutAction::ToggleThirdPerson)+(controller.ThirdPerson() ? ": RTS" : ": THIRD PERSON");
    controls+=" | "+key(ShortcutAction::ToggleView)+(controller.ThirdPerson() ? ": FPS" : ": THIRD PERSON");
    controls+=" | "+key(ShortcutAction::ToggleUnitView)+": UNIT VIEW";
    std::string equipment="WHEEL: WEAPONS | "+key(ShortcutAction::Weapon6)+": C4 | "+key(ShortcutAction::Pickup)+": ITEM | "+key(ShortcutAction::Buy)+": BUY | "+key(ShortcutAction::BuyAmmo)+": AMMO";
    // The same control now dispatches the unit's primary melee or ranged attack.
    if(controller.ThirdPerson())equipment="LMB: UNIT ATTACK | WASD: MOVE | "+key(ShortcutAction::Pickup)+": ITEM | "+key(ShortcutAction::Buy)+": BUY";
    // Show the shared direct-contact mode continuously, including after the transient Q notice disappears.
    equipment+=" | "+key(ShortcutAction::ToggleAllyTargeting)+": ALLIES "+(controller.Settings().targetAllies ? "ON" : "OFF");
    std::string abilities=key(ShortcutAction::HealthBars)+": HEALTH BARS | "+key(ShortcutAction::Spell1)+"/"+key(ShortcutAction::Spell2)+"/"+key(ShortcutAction::Spell3)+"/"+key(ShortcutAction::Spell4)+": SPELLS | "+key(ShortcutAction::SelfCast)+": SELF";
    char squad[256];sprintf_s(squad,"%s: FIGHT | %s: FOLLOW | %s: RELEASE | SQUAD %u %s | %s: FREE AMMO | %s: ALL WEAPONS",
        key(ShortcutAction::SquadFight).c_str(),key(ShortcutAction::SquadFollow).c_str(),key(ShortcutAction::SquadRelease).c_str(),unsigned(controller.SquadCount()),controller.SquadCount() ? (controller.SquadPassive() ? "FOLLOW" : "COMBAT") : "",key(ShortcutAction::Refill).c_str(),key(ShortcutAction::AllWeapons).c_str());
    int hintRight=controller.MinimapVisible() ? mapLayout.x-mapLayout.padding : width-25;
    int columns=std::max(1,(hintRight-25)/hintGlyphWidth_);float hintY=27;
    for(const auto& group : {controls,equipment,abilities,std::string(squad)}) {
        for(const auto& row : HudHints::Wrap(group,columns)) {Text(25,hintY,row.c_str());hintY+=23;}
    }
    if (controller.RefillNotice()) Text(statusPad, statusY - statusSize - 16, controller.AmmoMessage(), statusFont_);
    if(controller.Settings().showSpellIcons && !unavailableView && !controller.Buying()) {
        std::array<std::string,4> keys;
        for(int i=0;i<4;++i)keys[i]=key(ShortcutAction(int(ShortcutAction::Spell1)+i));
        auto layout=SpellHudLayout::Calculate(width,height,controller.Settings().spellIconSizePercent);
        // Match labels to icon size so READY/countdowns do not overlap neighboring slots after resize.
        int size=std::clamp(layout.side/3,12,28);
        if(size!=spellFontSize_) {if(spellFont_)glDeleteLists(spellFont_,96);spellFont_=BitmapFont(dc,size);spellFontSize_=size;}
        spellHud_.Draw(controller.SpellHud(),keys,layout,controller.Settings().showSpellCooldowns,
            [&](float x,float y,const char* label){Text(x,y,label,spellFont_);});
    }
    // Horizontal CS units match MaxBunnySpeed, excluding vertical falling/jumping velocity.
    // Stack above notices/status so the optional counter never covers health or ammunition.
    if (controller.SpeedVisible()) {
        char speed[64];sprintf_s(speed,"SPEED %.0f u/s (CS)",controller.DisplaySpeed());
        int rows=1+(controller.RefillNotice() ? 1 : 0)+(*controller.Status().Label() ? 1 : 0);
        Text(statusPad,statusY-(statusSize+16)*rows,speed,statusFont_);
    }
    // Ground items advertise interaction without issuing a walk-to-item RTS order.
    if (controller.ItemNearby()) {controls=key(ShortcutAction::Pickup)+": PICK UP ITEM";Text(width*.5f-120,height*.72f,controls.c_str(),statusFont_);}
    // Explain blocked controls next to the status HUD while the corresponding Warcraft effect lasts.
    if (*controller.Status().Label()) {
        char effect[96]; sprintf_s(effect, "%s", controller.Status().Label());
        if (!controller.Status().immobilized && std::abs(controller.Status().speedScale - 1) >= 0.01f)
            sprintf_s(effect, "%s  %.0f%% SPEED", controller.Status().Label(), controller.Status().speedScale * 100);
        Text(statusPad, statusY - (statusSize + 16) * (controller.RefillNotice() ? 2 : 1), effect, statusFont_);
    }
    BuyMenuView::Draw(controller,width,height,window,[&](float x,float y,const char* text){Text(x,y,text);},
        [&](float x,float y,const char* title){Text(x,y,title,statusFont_);},
        [&](int slot,float x,float y,float w,float h) {
            auto item=BuyCatalog::Slot(slot);if (!item) return;
            // Private previews load lazily; absent optional art never prevents a purchase.
            if (!previewAttempted_[slot]) {
                previewAttempted_[slot]=true;
                previewLoaded_[slot]=buyPreviews_[slot].Load(root_+"\\assets\\buy\\"+item->cache+".wcg");
            }
            if (previewLoaded_[slot]) buyPreviews_[slot].DrawPreview(x,y,w,h);
        });
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_TEXTURE); glPopMatrix();
    glMatrixMode(oldMode); glPopAttrib();
    if (activeTexture) activeTexture(oldTexture);
    // Report submission decisions, not pixel visibility; sampled GL errors can originate in the host frame.
    if (DiagnosticLog::Due(lastDiagnostic_)) {
        wc3::Trace("health bars allies=%d enemies=%d candidates=%u submitted=%u distance=%.0f",
            controller.Settings().healthBars.allies,controller.Settings().healthBars.enemies,unsigned(bars.size()),barCount,controller.Settings().healthBars.distance);
        // Sample display submission with the same bounded cadence as existing overlay diagnostics.
        wc3::Trace("overlay speed visible=%d speedCS=%.1f",controller.SpeedVisible(),controller.DisplaySpeed());
        GLint viewport[4] = {}; GLboolean mask[4] = {};
        glGetIntegerv(GL_VIEWPORT, viewport); glGetBooleanv(GL_COLOR_WRITEMASK, mask);
        GLenum observedError = glGetError();
        wc3::Trace("overlay submitted size=%dx%d weapon=%d loaded=%d reticle=%s recoil=%.2f viewport=%d,%d,%d,%d colorMask=%d%d%d%d lineStipple=%d polygonStipple=%d observedGlError=%04X",
            width, height, index + 1, loaded_[index], unavailableView ? "hidden-unit" : controller.Scoped() ? "scope" : "hip-fire",
            controller.Recoil(), viewport[0], viewport[1], viewport[2], viewport[3], mask[0], mask[1], mask[2], mask[3],
            glIsEnabled(GL_LINE_STIPPLE), glIsEnabled(GL_POLYGON_STIPPLE), observedError);
        if (observedError != GL_NO_ERROR) wc3::LogError("OpenGL error observed after overlay=%04X (host origin possible)", observedError);
        wc3::Trace("player hp=%.1f ammo=%d reserve=%d contained=%d hidden=%d disabled=%d disarmed=%d status=%s",
            controller.Health(), controller.Ammo(), controller.Reserve(), controller.Status().contained, controller.Status().hidden,
            controller.Status().incapacitated, controller.Status().disarmed, controller.Status().Label());
    }
}
