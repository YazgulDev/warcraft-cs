#include "GraphicsApi.hpp"
#include "Overlay.hpp"
#include "EnergyBeamView.hpp"
#include "ViewmodelAnchor.hpp"
#include "ScopeView.hpp"
#include "GrenadeView.hpp"
#include "ReticleView.hpp"
#include "TargetMarkerView.hpp"
#include "ReticleDiagnostics.hpp"
#include "BuyMenuView.hpp"
#include "WeaponSelectionView.hpp"
#include "SettingsMenuView.hpp"
#include "NuclearView.hpp"
#include "FpsTextView.hpp"
#include "HudHints.hpp"
#include "HudRenderState.hpp"
#include "FixedFunctionState.hpp"
#include "GraphicsDiagnostics.hpp"
#include "InventoryHudView.hpp"
#include "../platform/NativeMinimap.hpp"
#include "../platform/DiagnosticLog.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

void Overlay::Font(int statusSize) {
    // Alpha atlases keep hints and resolution-scaled status text independent of WGL bitmap-list drivers.
    font_.Ensure(20);statusFont_.Ensure(statusSize);
    hintGlyphWidth_=font_.Advance();
}
void Overlay::Text(float x, float y, const char* text, const HudFont* font) {
    (font ? *font : font_).Draw(x,y,text);
}
void Overlay::ResetGraphics(bool deleteObjects) {
    grenadeWorld_.Reset(deleteObjects);rocketView_.Reset(deleteObjects);
    energyBeamArt_.Reset(deleteObjects);energyImpactArt_.Reset(deleteObjects);
    spellHud_.Reset(deleteObjects);
    inventoryIcons_.Reset(deleteObjects);unitFont_.Reset(deleteObjects);inventoryFont_.Reset(deleteObjects);
    sky_.Reset(deleteObjects);
    // GPU names belong to one context; never delete old numeric IDs in a replacement context.
    for (int i = 0; i < WeaponSlots::Count; ++i) { guns_[i].Reset(deleteObjects); loaded_[i] = attempted_[i] = false; }
    // Menu models obey the same context ownership rules as first-person weapons.
    for (int i=0;i<WeaponSlots::Count;++i) { buyPreviews_[i].Reset(deleteObjects);previewLoaded_[i]=previewAttempted_[i]=false; }
    settingsFont_.Reset(deleteObjects);
    font_.Reset(deleteObjects);statusFont_.Reset(deleteObjects);spellFont_.Reset(deleteObjects);itemName_.Reset(deleteObjects);combatView_.Reset(deleteObjects);
}
void Overlay::ContextDeleted(HGLRC context) {
    if (context_ != context) return;
    ResetGraphics(false); context_ = nullptr;
    wc3::Log("Overlay context deleted; GPU names forgotten");
}
void Overlay::Draw(HDC dc, const ShooterController& controller,NativeMinimap& minimap,NativePortrait& portrait,const std::vector<UnitHealthBar>& bars,
    const WorldLabelView& barView,float nearPlane,float farPlane) {
    // Explain early returns as well as submitted geometry when investigating a missing aiming mark.
    if (!controller.Visible()) { ReticleDiagnostics::Skip("fps-inactive"); return; }
    // Clean unit view still receives physical grenade smoke/blindness; its HUD and arms stay hidden below.
    if (!wglGetCurrentContext()) { ReticleDiagnostics::Skip("no-gl-context"); return; }
    HGLRC current = wglGetCurrentContext();
    // Boundaries observe transient failures while the scope restores any pre-existing driver observer.
    GraphicsDiagnostics graphics(controller.Settings().logging);
    graphics.Phase("context-resources");
    if (context_ != current) {
        ResetGraphics(false); context_ = current;
        wc3::Log("Overlay context acquired: %p", current);
        // Record the actual driver renderer for reports where the overlay initializes but primitives disappear.
        wc3::Log("Overlay GL vendor=%s renderer=%s version=%s lineStipple=%d",
            glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION), glIsEnabled(GL_LINE_STIPPLE));
    } else if (refreshPending_) {
        // Keep surviving resources: blind deletion after Alt-Tab can touch IDs recycled by the game.
        bool valid=settingsFont_.Valid() && font_.Valid() && statusFont_.Valid() && spellFont_.Valid() && grenadeWorld_.Valid() && rocketView_.Valid();
        valid=valid && sky_.Valid() && spellHud_.Valid() && inventoryIcons_.Valid() && unitFont_.Valid() && inventoryFont_.Valid() && energyBeamArt_.Valid() && energyImpactArt_.Valid() && itemName_.Valid() && combatView_.Valid();
        for (int i=0;i<WeaponSlots::Count;++i) if (loaded_[i] && !guns_[i].TexturesValid()) valid=false;
        for (int i=0;i<WeaponSlots::Count;++i) if (previewLoaded_[i] && !buyPreviews_[i].TexturesValid()) valid=false;
        if (!valid) { ResetGraphics(false); wc3::Log("Overlay lost resources forgotten after focus return"); }
        else wc3::Log("Overlay resources retained after focus return");
    }
    refreshPending_ = false;
    // Imports can keep the same path across maps; reload those textures in the active rendering context.
    if(spellIconsRefresh_) {spellHud_.Reset(true);inventoryIcons_.Reset(true);spellIconsRefresh_=false;}
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
    // One cadence covers raw host state, the post-weapon handoff and the prepared/completed flat HUD.
    bool sampledHud=hudDiagnostics_.Begin(controller.UnitFirstPerson() ? "clean-unit-view" : controller.ThirdPerson() ? "third-person" : "weapon-first-person",width,height);
    // Capture the raw host first, then isolate world effects and weapons before the nested HUD scope.
    FixedFunctionState fixedFunction;
    graphics.Phase("overlay-state");
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
    if (controller.Settings().skyVisible && controller.Settings().csSky && !controller.Status().contained && !controller.Status().hidden)
        sky_.Draw(root_,controller.SkyName(),controller.ViewYaw(),controller.ViewPitch(),controller.ViewFov(),float(width)/height);
    auto drawSettings=[&] {
        if(!controller.SettingsPanel().Open())return;
        settingsFont_.Ensure(std::clamp(height/38,14,24));POINT cursor={};GetCursorPos(&cursor);ScreenToClient(WindowFromDC(dc),&cursor);
        SettingsMenuView::Draw(controller.SettingsPanel(),width,height,cursor.x,cursor.y,[&](float x,float y,const char* label){Text(x,y,label,&settingsFont_);});
    };
    auto drawTextInput=[&] {FpsTextView::Draw(controller.TextEntry(),controller.ChatMessage(),controller.ChatMessageVisible(),width,height,font_);};
    NuclearView::World(controller.NuclearBomb(),barView,nearPlane,farPlane);
    // World depth still exists here; the first-person weapon clears it in the next pass.
    if(controller.UnitFirstPerson()) {
        graphics.Phase("clean-view-effects");
        itemName_.Draw(controller.HoveredItemVisible(barView.eye) ? controller.HoveredItemName() : std::string{},width,height,std::clamp(height/45,16,30));
        HudDiagnostics::Omitted("clean-unit-view",sampledHud);
        if(!controller.Status().contained && !controller.Status().hidden) {
            grenadeWorld_.Draw(root_,controller.GrenadeVisuals(),barView,width,height,nearPlane,farPlane);
            rocketView_.Draw(root_,controller.RocketVisuals(),controller.LaserVisible(),controller.LaserPoint(),barView,width,height,nearPlane,farPlane);
            GrenadeView::Draw(controller.GrenadeVisuals(),barView,width,height,nearPlane,farPlane);
            GrenadeView::Flash(controller.FlashOpacity(),width,height);
        }
        drawSettings();drawTextInput();GrenadeView::Flash(controller.NuclearBomb().Flash(),width,height);
        glMatrixMode(GL_TEXTURE);glPopMatrix();glMatrixMode(oldMode);glPopAttrib();
        if(activeTexture)activeTexture(oldTexture);
        return;
    }
    // Load the held model before world effects so its actual animated attachment can anchor an energy beam.
    int index=controller.WeaponIndex();float projectionScale=controller.Settings().weaponModelScale[index];
    graphics.Phase("weapon-resource-load");
    float animationSeconds=controller.AnimationTime(); // Attachment and mesh must sample one identical animation phase.
    if(!controller.Settings().warcraftOnly && !attempted_[index]) {
        attempted_[index]=true;loaded_[index]=guns_[index].Load(root_+"\\assets\\"+WeaponSlots::Caches[index]+".wcg");
        if(!loaded_[index])wc3::LogError("graphics resource load failed role=held-model slot=%d path=%s\\assets\\%s.wcg",index,root_.c_str(),WeaponSlots::Caches[index]);
    }
    graphics.Phase("world-effects");
    // Copied target/casing pools render in world depth, independently of the current weapon's private depth pass.
    if(!controller.Status().contained && !controller.Status().hidden)
        combatView_.Draw(controller.Visuals(),barView,width,height,nearPlane,farPlane);
    unsigned barCount=0;
    if(!controller.Status().contained && !controller.Status().hidden && !controller.Buying()) {
        std::vector<UnitHealthBar> visibleBars;
        for(const auto& bar:bars)if(controller.VisibleThroughSmoke(barView.eye,bar.position))visibleBars.push_back(bar);
        barCount=HealthBarRenderer::Draw(visibleBars,barView,width,height,nearPlane,farPlane);
    }
    if(!controller.Status().contained && !controller.Status().hidden) {
        if(!controller.ThirdPerson() && !controller.Buying() && WeaponSlots::Energy(index)) {
            auto beam=controller.Beam();std::array<float,3> anchor;
            if(beam.tick && loaded_[index] && guns_[index].MuzzlePoint(controller.Animation(),animationSeconds,controller.ShotSerial(),anchor,controller.IdleAnimation()))
                beam.start=ViewmodelAnchor::World(anchor,controller.WeaponPose(),projectionScale,barView,float(width)/height,nearPlane);
            GLuint beamArt=0,impactArt=0;
            if(beam.tick && beam.gluon && beam.effects && beam.ownedSprites) {
                beamArt=energyBeamArt_.Frame(root_+"/assets/effects/egon_beam.wes",0);
                impactArt=energyImpactArt_.Frame(root_+"/assets/effects/egon_impact.wes",GetTickCount()/50);
            }
            EnergyBeamView::Draw(beam,barView,nearPlane,farPlane,beamArt,impactArt);
        }
        grenadeWorld_.Draw(root_,controller.GrenadeVisuals(),barView,width,height,nearPlane,farPlane);
        rocketView_.Draw(root_,controller.RocketVisuals(),controller.LaserVisible(),controller.LaserPoint(),barView,width,height,nearPlane,farPlane);
        GrenadeView::Draw(controller.GrenadeVisuals(),barView,width,height,nearPlane,farPlane);
    }
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    graphics.Phase("held-model");
    // Slightly tighten USP/AWP framing while preserving the original lower-edge arm origins.
    float top = std::tan(45.0f * 0.01745329252f) * projectionScale; float side = top * width / height;
    glFrustum(-side, side, -top, top, 1, 200);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    bool heldModel=!controller.NuclearSelected() && !controller.ThirdPerson() && loaded_[index] && !controller.Status().contained && !controller.Status().hidden && (index!=WeaponSlots::C4 || controller.Ammo()>0);
    HudDiagnostics::Weapon(index,controller.ShotSerial(),controller.Animation(),animationSeconds,loaded_[index],heldModel,controller.Scoped(),
        controller.Status().contained,controller.Status().hidden);
    const auto& visuals=controller.Settings().viewmodel;
    float flash=muzzleFlash_.Sample(controller.ShotSerial(),heldModel && controller.ShowMuzzleFlash(),GetTickCount(),visuals.flashMilliseconds);
    if (heldModel && (!controller.Scoped() || flash>0)) {
        // Give the weapon its own depth pass: back faces must not paint over the skin.
        glDepthMask(GL_TRUE); glClearDepth(1); glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthRange(0, 1);
        // Preserve each model's original arm origin instead of exposing detached elbow ends.
        const auto& pose=controller.WeaponPose();
        glTranslatef(pose.x,pose.y,pose.z);glRotatef(pose.roll,0,0,1);
        guns_[index].Draw(controller.Animation(),animationSeconds,flash,visuals.flashScale,controller.ShotSerial(),!controller.Scoped(),controller.IdleAnimation());
        glDisable(GL_DEPTH_TEST);
    }
    bool unavailableView = controller.Status().contained || controller.Status().hidden;
    HudDiagnostics::State("after-weapon",sampledHud);
    graphics.Phase("flat-hud");
    {
    // Re-establish all flat-HUD state after the world/weapon passes; restore it before the outer host scope.
    HudRenderState hudState;
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glDisable(GL_TEXTURE_2D);
    HudDiagnostics::State("prepared",sampledHud);
    // A swallowed/transported unit has no world viewpoint: hide equipment and retain the health/status HUD.
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
    if(!unavailableView && !controller.Buying() && controller.ThirdPerson() && controller.Settings().thirdPerson.showTargetIndicator && controller.VisibleThroughSmoke(barView.eye,controller.AimTarget().position))
        TargetMarkerView::Draw(controller.AimTarget(),barView,width,height,nearPlane,farPlane);
    // Scale the bottom HUD from a readable 42-pixel font at 1080p, with safe edge padding.
    int statusSize = std::clamp(int(std::lround(height * (42.0f / 1080.0f))), 26, 84);
    bool unitHudVisible=controller.Settings().showUnitHud && controller.UnitHud().actor;
    // Choose each font's size once per frame; separate inventory labels avoid rebuilding glyph atlases.
    if(unitHudVisible)statusSize=std::max(18,statusSize*2/3);
    Font(statusSize);
    controller.Hits().Draw(float(width), float(height));
    if(!unavailableView && !controller.Buying()) {
        itemName_.Draw(controller.HoveredItemVisible(barView.eye) ? controller.HoveredItemName() : std::string{},width,height,std::clamp(height/45,16,30));
    }

    // Money formatting follows the active purchase currency in both HUD layouts.
    char status[160]; sprintf_s(status, "HP %.0f   %s   %d / %d   %s", controller.Health(),
        controller.WeaponData(index).name, controller.Ammo(), controller.Reserve(),controller.MoneyText().c_str());
    // Melee equipment has no magazine; show only its name instead of a misleading empty-ammo counter.
    if (WeaponSlots::Melee(index)) sprintf_s(status, "HP %.0f   %s   %s", controller.Health(), controller.WeaponData(index).name,controller.MoneyText().c_str());
    // Hero combat has no magazine or CS weapon label; expose the mode and live native attack instead.
    if(controller.ThirdPerson())sprintf_s(status,"HP %.0f   UNIT ATTACK %.0f   %s",controller.Health(),
        controller.Status().attackDamage*controller.Settings().thirdPerson.damageMultiplier,controller.MoneyText().c_str());
    float statusPad = statusSize * 0.75f, statusY = height - statusPad;
    auto unitLayout=UnitHudLayout::Calculate(width,height,controller.Settings().unitHudScalePercent);
    if(unitHudVisible) {
        // The new health block reserves the lower left; equipment/notices stack above it.
        statusY=float(unitLayout.y-16);
        unitFont_.Ensure(std::clamp(int(unitLayout.portrait*.19f),12,32));
        UnitHudView::Draw(controller.UnitHud(),unitLayout,portrait,[&](float x,float y,const char* label){Text(x,y,label,&unitFont_);});
        if(controller.ThirdPerson())sprintf_s(status,"UNIT ATTACK %.0f   %s",controller.Status().attackDamage*controller.Settings().thirdPerson.damageMultiplier,controller.MoneyText().c_str());
        else if(WeaponSlots::Melee(index))sprintf_s(status,"%s   %s",controller.WeaponData(index).name,controller.MoneyText().c_str());
        else sprintf_s(status,"%s   %d / %d   %s",controller.WeaponData(index).name,controller.Ammo(),controller.Reserve(),controller.MoneyText().c_str());
    }
    if(controller.NuclearSelected())sprintf_s(status,"NUCLEAR BOMB x1   %s",controller.MoneyText().c_str());
    glColor4f(1.0f, 0.85f, 0.3f, 1); Text(statusPad, statusY, status, &statusFont_);
    // Installation progress and fuse are independent of which weapon is currently equipped.
    if (controller.PlantProgress() > 0) {
        char message[80]; sprintf_s(message, "PLANTING C4  %.0f%%", controller.PlantProgress() / 3 * 100);
        Text(width * 0.5f - 180, height * 0.65f, message, &statusFont_);
    }
    if (controller.BombRemaining() >= 0) {
        char message[80]; sprintf_s(message, "C4 x%u  %.0fs",unsigned(controller.BombCount()),double(std::ceil(controller.BombRemaining())));
        Text(float(width - statusSize * 9), statusY, message, &statusFont_);
    }
    // Compact hints reflect the configured keys and wrap before the native minimap.
    auto key=[&](ShortcutAction action) {return controller.Shortcut(action);};
    auto mapLayout=MinimapLayout::Calculate(width,height,controller.Settings().minimapSizePercent);
    // Backend-owned native captures share the same minimap placement and controls.
    if(controller.MinimapVisible())minimap.Draw(mapLayout);
    std::string controls="Warcraft CS by Yazgul | ENTER: CHAT | "+key(ShortcutAction::ToggleFPS)+": RTS | "+key(ShortcutAction::ReloadSettings)+": RELOAD INI | "+key(ShortcutAction::SettingsMenu)+": SETTINGS | "+key(ShortcutAction::Minimap)+": MINIMAP | "+key(ShortcutAction::Speed)+": SPEED | "+key(ShortcutAction::Fog)+": FOG";
    // The dedicated key exposes direct entry/RTS exit independently of the active view switch.
    controls+=" | "+key(ShortcutAction::ToggleThirdPerson)+(controller.ThirdPerson() ? ": RTS" : ": THIRD PERSON");
    controls+=" | "+key(ShortcutAction::ToggleView)+(controller.ThirdPerson() ? (controller.Settings().warcraftOnly ? ": UNIT VIEW" : ": FPS") : ": THIRD PERSON");
    controls+=" | "+key(ShortcutAction::ToggleUnitView)+": UNIT VIEW";
    std::string equipment="WHEEL: WEAPONS | "+(controller.Settings().weaponSelectionMenu ? key(ShortcutAction::Weapon6)+": HALF-LIFE" : key(ShortcutAction::Weapon6)+": C4")+" | "+key(ShortcutAction::Pickup)+": ITEM | "+key(ShortcutAction::Buy)+": BUY | "+key(ShortcutAction::BuyAmmo)+": AMMO";
    // The same control now dispatches the unit's primary melee or ranged attack.
    if(controller.ThirdPerson())equipment="LMB: UNIT ATTACK | WASD: MOVE | WHEEL: CAMERA | "+key(ShortcutAction::Pickup)+": ITEM / SHOP";
    if(controller.Settings().warcraftOnly)equipment="LMB: UNIT ATTACK | WASD: MOVE | WHEEL: CAMERA | "+key(ShortcutAction::Pickup)+": ITEM / SHOP";
    // Show the shared direct-contact mode continuously, including after the transient Q notice disappears.
    equipment+=" | "+key(ShortcutAction::ArmyAI)+": ARMY AI "+(controller.ArmyAIActive() ? "ON" : "OFF");
    equipment+=" | "+key(ShortcutAction::ToggleAllyTargeting)+": ALLIES "+(controller.Settings().targetAllies ? "ON" : "OFF");
    if(controller.Settings().inventoryControls)equipment+=" | "+key(ShortcutAction::InventoryNumbers)+": ITEMS "+(controller.ItemControls().Numbers() ? "ON" : "OFF")+" | "+key(ShortcutAction::InventoryPanel)+": INVENTORY";
    std::string abilities=key(ShortcutAction::HealthBars)+": HEALTH BARS | "+key(ShortcutAction::Spell1)+"/"+key(ShortcutAction::Spell2)+"/"+key(ShortcutAction::Spell3)+"/"+key(ShortcutAction::Spell4)+": SPELLS | "+key(ShortcutAction::SelfCast)+": SELF";
    char squad[256];sprintf_s(squad,"%s: FIGHT | %s: FOLLOW | %s: RELEASE | SQUAD %u %s | %s: FREE AMMO | %s: ALL WEAPONS",
        key(ShortcutAction::SquadFight).c_str(),key(ShortcutAction::SquadFollow).c_str(),key(ShortcutAction::SquadRelease).c_str(),unsigned(controller.SquadCount()),controller.SquadCount() ? (controller.SquadPassive() ? "FOLLOW" : "COMBAT") : "",key(ShortcutAction::Refill).c_str(),key(ShortcutAction::AllWeapons).c_str());
    // Reduced clients advertise only available actions; native spells and squad controls remain visible.
    if(controller.Settings().warcraftOnly || controller.ThirdPerson()) {
        sprintf_s(squad,"%s: FIGHT | %s: FOLLOW | %s: RELEASE | SQUAD %u %s",
            key(ShortcutAction::SquadFight).c_str(),key(ShortcutAction::SquadFollow).c_str(),key(ShortcutAction::SquadRelease).c_str(),
            unsigned(controller.SquadCount()),controller.SquadCount() ? (controller.SquadPassive() ? "FOLLOW" : "COMBAT") : "");
    }
    if(controller.SquadReticleOrdersEnabled())strcat_s(squad," | RMB: ALL ATTACK / CLEAR");
    // Armed F6 shows both the rebindable mode switch and its current session choice.
    if(controller.Settings().squadReticleAttack && !controller.ThirdPerson())
        equipment+=" | "+key(ShortcutAction::SquadOrders)+": RMB "+(controller.SquadOrdersInFPS() ? "ORDERS" : "WEAPON");
    int hintRight=controller.MinimapVisible() ? mapLayout.x-mapLayout.padding : width-25;
    int columns=std::max(1,(hintRight-25)/hintGlyphWidth_);float hintY=27;
    if(!controller.Buying())for(const auto& group : {controls,equipment,abilities,std::string(squad)}) {
        for(const auto& row : HudHints::Wrap(group,columns)) {Text(25,hintY,row.c_str());hintY+=23;}
    }
    if (controller.RefillNotice()) Text(statusPad, statusY - statusSize - 16, controller.AmmoMessage(), &statusFont_);
    if((controller.Settings().showInventory || controller.InventoryOpen() || controller.ItemControls().Numbers()) && !unavailableView && !controller.Buying()) {
        auto layout=InventoryHudLayout::Calculate(width,height,controller.Settings().spellIconSizePercent,controller.Settings().showSpellIcons,controller.InventoryOpen());
        inventoryFont_.Ensure(std::clamp(layout.side/3,12,24));
        InventoryHudView::Draw(controller.UnitHud(),layout,inventoryIcons_,controller.ItemControls(),[&](float x,float y,const char* label){Text(x,y,label,&inventoryFont_);});
    }
    if(controller.Settings().showSpellIcons && !unavailableView && !controller.Buying()) {
        std::array<std::string,4> keys;
        for(int i=0;i<4;++i)keys[i]=key(ShortcutAction(int(ShortcutAction::Spell1)+i));
        auto layout=SpellHudLayout::Calculate(width,height,controller.Settings().spellIconSizePercent);
        // Match labels to icon size so READY/countdowns do not overlap neighboring slots after resize.
        int size=std::clamp(layout.side/3,12,28);
        spellFont_.Ensure(size);
        spellHud_.Draw(controller.SpellHud(),keys,layout,controller.Settings().showSpellCooldowns,
            [&](float x,float y,const char* label){Text(x,y,label,&spellFont_);});
    }
    // Horizontal CS units match MaxBunnySpeed, excluding vertical falling/jumping velocity.
    // Stack above notices/status so the optional counter never covers health or ammunition.
    if (controller.SpeedVisible()) {
        char speed[64];sprintf_s(speed,"SPEED %.0f u/s (CS)",controller.DisplaySpeed());
        int rows=1+(controller.RefillNotice() ? 1 : 0)+(*controller.Status().Label() ? 1 : 0);
        Text(statusPad,statusY-(statusSize+16)*rows,speed,&statusFont_);
    }
    // Ground items advertise interaction without issuing a walk-to-item RTS order.
    if (controller.ItemNearby()) {controls=key(ShortcutAction::Pickup)+": PICK UP "+(controller.NearbyWeaponName().empty() ? "ITEM" : controller.NearbyWeaponName());Text(width*.5f-120,height*.72f,controls.c_str(),&statusFont_);}
    // Explain blocked controls next to the status HUD while the corresponding Warcraft effect lasts.
    if (*controller.Status().Label()) {
        char effect[96]; sprintf_s(effect, "%s", controller.Status().Label());
        if (!controller.Status().immobilized && std::abs(controller.Status().speedScale - 1) >= 0.01f)
            sprintf_s(effect, "%s  %.0f%% SPEED", controller.Status().Label(), controller.Status().speedScale * 100);
        Text(statusPad, statusY - (statusSize + 16) * (controller.RefillNotice() ? 2 : 1), effect, &statusFont_);
    }
    // Both menus share one lazy preview cache; missing art keeps the text selection usable.
    auto weaponPreview=[&](int slot,float x,float y,float w,float h) {
        auto item=BuyCatalog::Slot(slot);if(!item)return;
        if(!previewAttempted_[slot]) {
            previewAttempted_[slot]=true;
            previewLoaded_[slot]=buyPreviews_[slot].Load(root_+"\\assets\\buy\\"+item->cache+".wcg");
            if(!previewLoaded_[slot])wc3::LogError("graphics resource load failed role=buy-preview slot=%d path=%s\\assets\\buy\\%s.wcg",slot,root_.c_str(),item->cache);
        }
        if(previewLoaded_[slot])buyPreviews_[slot].DrawPreview(x,y,w,h);
    };
    BuyMenuView::Draw(controller,width,height,window,[&](float x,float y,const char* text){Text(x,y,text);},
        [&](float x,float y,const char* title){Text(x,y,title,&statusFont_);},weaponPreview);
    WeaponSelectionView::Draw(controller,width,height,[&](float x,float y,const char* text){Text(x,y,text);},weaponPreview);
    InventoryHudView::Cursor(window,controller.UnitHud(),InventoryHudLayout::Calculate(width,height,controller.Settings().spellIconSizePercent,controller.Settings().showSpellIcons,true),inventoryIcons_,controller.ItemControls());
    if(controller.NuclearSelected()) {
        NuclearView::Held(width,height);glColor4f(1,.65f,.2f,1);
        Text(width*.5f-240,height*.70f,"NUCLEAR BOMB: LMB LAUNCH / RMB HOLSTER");
    }
    if(controller.NuclearBomb().Remaining()>=0){char message[80];sprintf_s(message,"NUCLEAR STRIKE IN %.1f",controller.NuclearBomb().Remaining());glColor4f(1,.4f,.15f,1);Text(width*.5f-140,height*.42f,message);}
    drawSettings();drawTextInput();
    GrenadeView::Flash(controller.NuclearBomb().Flash(),width,height);
    GrenadeView::Flash(controller.FlashOpacity(),width,height); // Blindness covers hands, scope and HUD together.
    HudDiagnostics::State("completed",sampledHud);
    }
    graphics.Phase("restore-host");
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
        // Submission without GL errors cannot establish that a font atlas or native HUD snapshot exists.
        GLint drawBuffer=0;glGetIntegerv(GL_DRAW_BUFFER,&drawBuffer);
        wc3::Trace("HUD ready hintFont=%d statusFont=%d unitFont=%d spellFont=%d inventoryFont=%d actor=%08X enabled=%d warcraftOnly=%d drawBuffer=%04X",
            font_.Ready(),statusFont_.Ready(),unitFont_.Ready(),spellFont_.Ready(),inventoryFont_.Ready(),
            controller.UnitHud().actor,controller.Settings().showUnitHud,controller.Settings().warcraftOnly,drawBuffer);
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
