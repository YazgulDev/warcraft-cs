#pragma once
#include <cstdint>

// Borrow the native preselection presentation only while the unarmed F9 view owns input.
namespace NativeHoverView {
using HookInstaller=bool (*)(void*,void*,void**);
bool Install(uintptr_t base,HookInstaller hook);
void Update(uintptr_t ui,bool hide);
void Reset(); // Map replacement invalidates the old UI; never restore into its freed storage.
}
