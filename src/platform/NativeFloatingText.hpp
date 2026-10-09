#pragma once
#include "../presentation/WorldLabelView.hpp"
#include <cstdint>

// Filter render submission only; native tag creation, lifetime and gold remain intact.
namespace NativeFloatingText {
using HookInstaller=bool (*)(void*,void*,void**);
bool Install(uintptr_t base, HookInstaller hook);
void SetView(const WorldLabelView& view);
}
