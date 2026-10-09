#pragma once
#include "../combat/WeaponSlots.hpp"
#include "GunMesh.hpp"
#include "../application/ShooterController.hpp"
#include "SkyView.hpp"

class Overlay {
public:
    void Configure(const char* root) { root_ = root; }
    void Draw(HDC dc, const ShooterController& controller);
    void ContextDeleted(HGLRC context);
    void RefreshAfterFocus() { refreshPending_ = true; }
private:
    void ResetGraphics(bool deleteObjects);
    void Text(float x, float y, const char* text, GLuint font = 0);
    void Font(HDC dc, int statusSize);
    std::string root_;
    GunMesh guns_[WeaponSlots::Count];
    GunMesh buyPreviews_[WeaponSlots::Count];
    bool previewLoaded_[WeaponSlots::Count]={},previewAttempted_[WeaponSlots::Count]={};
    bool loaded_[WeaponSlots::Count] = {}, attempted_[WeaponSlots::Count] = {};
    GLuint font_ = 0;
    GLuint statusFont_ = 0;
    int statusFontSize_ = 0;
    HGLRC context_ = nullptr;
    bool refreshPending_ = false;
    SkyView sky_;
};
