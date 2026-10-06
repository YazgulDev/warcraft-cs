#include "FullscreenView.hpp"
#include "WarcraftApi.hpp"
#include <cstring>

bool FullscreenView::Configure(uintptr_t base) {
    const unsigned char signature[] = {0x53,0x8B,0x5C,0x24,0x08};
    if (memcmp(reinterpret_cast<void*>(base + 0x606770), signature, sizeof(signature))) return false;
    base_ = base; return true;
}
void FullscreenView::Set(uintptr_t frame, int point, const Anchor& anchor) {
    using SetPoint = void (__fastcall*)(uintptr_t, uintptr_t, int, uintptr_t, int, float, float, int);
    reinterpret_cast<SetPoint>(base_ + 0x606770)(frame, 0, point, anchor.parent, anchor.relative, anchor.x, anchor.y, 1);
}
void FullscreenView::Reset() {
    // A destroyed map's frame pointers must never be restored into the next map.
    ui_ = frame_ = 0; expanded_ = false;
}
void FullscreenView::Update(uintptr_t ui, bool visible) {
    if (!base_ || !ui) return;
    if (ui_ != ui) { Reset(); ui_ = ui; }
    if (visible && !expanded_) {
        frame_ = *reinterpret_cast<uintptr_t*>(ui + 0x3BC) + 0xB4;
        auto read = [this](int point) {
            uintptr_t data = *reinterpret_cast<uintptr_t*>(frame_ + 8 + point * 4);
            Anchor result;
            if (data) memcpy(&result, reinterpret_cast<void*>(data + 4), sizeof(result));
            return result;
        };
        top_ = read(2); bottom_ = read(6); expanded_ = true;
        wc3::Log("FPS world viewport expanded");
    }
    if (visible && expanded_) {
        // Override cinematic letterboxing after ShowInterface(false) adjusts its anchors.
        Set(frame_, 2, {ui + 0xB4, 2, 0, 0}); Set(frame_, 6, {ui + 0xB4, 6, 0, 0});
    } else if (expanded_) {
        if (top_.parent) Set(frame_, 2, top_);
        if (bottom_.parent) Set(frame_, 6, bottom_);
        expanded_ = false; wc3::Log("RTS world viewport restored");
    }
}
