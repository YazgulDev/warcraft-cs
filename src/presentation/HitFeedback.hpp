#pragma once
#include <windows.h>

// Reticle damage confirmation stays separate from the bounded target-local critical/blood pools.
class HitFeedback {
public:
    void Record(bool headshot=false);
    bool Headshot() const {return headshotActive_ && GetTickCount()-headshotTick_<650;}
    void Clear();
    void Draw(float width, float height) const;
private:
    DWORD tick_ = 0;
    bool active_ = false;
    // Separate expiry prevents later body hits from extending an earlier headshot label.
    DWORD headshotTick_=0;
    bool headshotActive_=false;
};
