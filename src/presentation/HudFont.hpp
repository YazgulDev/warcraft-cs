#pragma once
#include <windows.h>
#include <gl/GL.h>
#include <array>

// Owns one context's alpha atlas; Overlay controls disposal when WGL contexts change.
class HudFont {
public:
    HudFont()=default;
    HudFont(const HudFont&)=delete;
    HudFont& operator=(const HudFont&)=delete;
    bool Ensure(int height);
    void Draw(float x, float baseline, const char* text) const;
    void Reset(bool deleteObjects);
    bool Valid() const { return !texture_ || glIsTexture(texture_) == GL_TRUE; }
    int Advance() const { return advance_; }
private:
    struct Glyph {
        int width=0, height=0, left=0, top=0, advance=0;
        float u0=0, v0=0, u1=0, v1=0;
    };
    std::array<Glyph,95> glyphs_{};
    GLuint texture_=0;
    int height_=0, advance_=12;
};
