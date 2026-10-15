#pragma once
#include <windows.h>
#include <cstdint>

// Own the native pre-presentation boundary; HUD composition does not depend on the swap API.
namespace NativeFramePresentation {
using Hook=bool (*)(void*,void*,void**);
using Frame=void (*)(HDC,const char*);
bool Install(uintptr_t gameBase,Hook hook,Frame frame);
}
