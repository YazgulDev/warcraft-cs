#include "runtime/PluginRuntime.hpp"

// Keep the published source entry point stable for older launchers' archive validation.
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    return reason == DLL_PROCESS_ATTACH ? PluginRuntime::Attach(module) : TRUE;
}
