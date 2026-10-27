#pragma once
#include <windows.h>
#include <string>

// One sampled HUD frame shares its cadence across all render stages; queries never read framebuffer pixels.
class HudDiagnostics {
public:
    bool Begin(const char* mode,int width,int height);
    static void State(const char* phase,bool sampled);
    static void Omitted(const char* reason,bool sampled);
    static void Font(const char* role,const char* phase,int requested,int width,int height,
        unsigned texture,unsigned long error,int glyph,int coverage);
    // Decisions/accepted-round changes are immediate; ordinary unchanged frames stay sampled.
    static void Weapon(int slot,unsigned serial,const char* animation,float seconds,bool loaded,bool held,bool scoped,bool contained,bool hidden);
private:
    DWORD previous_=0;
    HGLRC context_=nullptr;
    std::string mode_;
};
