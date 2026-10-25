#include <windows.h>
#include <cstring>

// Player setup fills this read-only data reservation with the selected owner's exact Miles exports.
// Keeping it separate lets both host versions share compiled loader code without shipping either retail DLL.
#pragma section(".wcsfwd",read)
extern "C" __declspec(allocate(".wcsfwd")) unsigned char WarcraftCsForwarderBuffer[131072]={};
#pragma comment(linker,"/include:_WarcraftCsForwarderBuffer")

static char modPath[MAX_PATH];
static DWORD WINAPI LoadMod(void*) {
    // Load the extension outside the loader lock; sound exports forward to the untouched library.
    LoadLibraryA(modPath);
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        GetModuleFileNameA(module, modPath, MAX_PATH);
        char* separator = strrchr(modPath, '\\');
        if (separator) strcpy_s(separator + 1, MAX_PATH - (separator + 1 - modPath), "WarcraftCS.mix");
        HANDLE thread = CreateThread(nullptr, 0, LoadMod, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
