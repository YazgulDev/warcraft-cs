#pragma once
#include "../combat/WeaponSlots.hpp"
#include "GunMesh.hpp"
#include "../application/ShooterController.hpp"
#include "SkyView.hpp"
#include "HealthBarRenderer.hpp"
#include "SpellHudView.hpp"
#include "HudFont.hpp"
#include "UnitHudView.hpp"
class NativeMinimap;
class NativePortrait;

class Overlay {
public:
    void Configure(const char* root) { root_ = root; }
    void Draw(HDC dc, const ShooterController& controller,NativeMinimap& minimap,NativePortrait& portrait,const std::vector<UnitHealthBar>& bars,
        const WorldLabelView& barView,float nearPlane,float farPlane);
    void ContextDeleted(HGLRC context);
    void RefreshAfterFocus() { refreshPending_ = true; }
    void MapChanged() {spellIconsRefresh_=true;}
private:
    void ResetGraphics(bool deleteObjects);
    SpellHudView spellHud_;
    HudIconCache inventoryIcons_;
    bool spellIconsRefresh_=false;
    void Text(float x, float y, const char* text, const HudFont* font = nullptr);
    void Font(int statusSize);
    std::string root_;
    GunMesh guns_[WeaponSlots::Count];
    GunMesh buyPreviews_[WeaponSlots::Count];
    bool previewLoaded_[WeaponSlots::Count]={},previewAttempted_[WeaponSlots::Count]={};
    bool loaded_[WeaponSlots::Count] = {}, attempted_[WeaponSlots::Count] = {};
    HudFont font_,statusFont_,spellFont_,unitFont_,inventoryFont_;
    int hintGlyphWidth_ = 12;
    HGLRC context_ = nullptr;
    bool refreshPending_ = false;
    DWORD lastDiagnostic_ = 0;
    SkyView sky_;
};
