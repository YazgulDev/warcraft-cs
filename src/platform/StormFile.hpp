#pragma once
#include <windows.h>
#include <vector>

namespace StormFile {
// Storm searches the active map before the installed archives, including custom imports.
inline bool Read(const char* path, std::vector<unsigned char>& bytes, DWORD limit) {
    HMODULE storm = GetModuleHandleA("Storm.dll");
    if (!storm) return false;
    auto open = reinterpret_cast<BOOL(WINAPI*)(const char*, HANDLE*)>(GetProcAddress(storm, MAKEINTRESOURCEA(267)));
    auto size = reinterpret_cast<DWORD(WINAPI*)(HANDLE, DWORD*)>(GetProcAddress(storm, MAKEINTRESOURCEA(265)));
    auto read = reinterpret_cast<BOOL(WINAPI*)(HANDLE, void*, DWORD, DWORD*, LONG)>(GetProcAddress(storm, MAKEINTRESOURCEA(269)));
    auto close = reinterpret_cast<BOOL(WINAPI*)(HANDLE)>(GetProcAddress(storm, MAKEINTRESOURCEA(253)));
    HANDLE file = nullptr;
    if (!open || !size || !read || !close || !open(path, &file)) return false;
    DWORD length = size(file, nullptr), got = 0;
    bool ok = length >= 4 && length <= limit;
    if (ok) {
        bytes.resize(length);
        ok = read(file, bytes.data(), length, &got, 0) && got == length;
    }
    close(file);
    return ok;
}
}
