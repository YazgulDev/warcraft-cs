#pragma once
#include <windows.h>

namespace PluginRuntime {
// The public DLL entry point delegates loader-safe startup to the runtime composition root.
BOOL Attach(HINSTANCE module);
}
