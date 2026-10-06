#pragma once
#include <cstdint>

// Classic UI frame anchors define the real world viewport, independently of the HUD.
class FullscreenView {
public:
    bool Configure(uintptr_t base);
    void Update(uintptr_t ui, bool visible);
    void Reset();
private:
    struct Anchor { uintptr_t parent = 0; int relative = 0; float x = 0, y = 0; };
    void Set(uintptr_t frame, int point, const Anchor& anchor);
    uintptr_t base_ = 0, ui_ = 0, frame_ = 0;
    Anchor top_, bottom_;
    bool expanded_ = false;
};
