#pragma once
#include <windows.h>
#include "GraphicsApi.hpp"
#include <array>

// Owns one context's alpha atlas; Overlay controls disposal when WGL contexts change.
class HudFont {
public:
    explicit HudFont(const char* role="unspecified"):role_(role) {}
    HudFont(const HudFont&)=delete;
    HudFont& operator=(const HudFont&)=delete;
    bool Ensure(int height);
    void Draw(float x, float baseline, const char* text) const;
    void Reset(bool deleteObjects);
    bool Valid() const { return !texture_ || glIsTexture(texture_) == GL_TRUE; }
    // An unallocated font is valid during lazy creation, but cannot render text yet.
    bool Ready() const { return texture_ && glIsTexture(texture_) == GL_TRUE; }
    int Advance() const { return advance_; }
private:
    bool Report(const char* phase,int requested,int width=0,int height=0,unsigned texture=0,unsigned long error=0,int glyph=-1,int coverage=-1);
    const char* role_;
    const char* lastPhase_="";
    DWORD lastReport_=0;
    HGLRC lastReportContext_=nullptr;
    struct Glyph {
        int width=0, height=0, left=0, top=0, advance=0;
        float u0=0, v0=0, u1=0, v1=0;
    };
    std::array<Glyph,351> glyphs_{};
    GLuint texture_=0;
    int height_=0, advance_=12;
};
