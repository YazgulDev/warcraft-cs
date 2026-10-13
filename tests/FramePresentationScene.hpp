#pragma once
#include <cstdint>

// Only disposable builds can select an alternate presentation API for the same native frame.
namespace FramePresentationScene {
using Hook=bool (*)(void*,void*,void**);
void Install(uintptr_t base,Hook hook);
void Alternate(bool enabled);
}
