#pragma once
#include <windows.h>
#include <string>
#include <cstdint>

// Submit a completed local line through the verified host edit box and chat/cheat/map-trigger handler.
namespace NativeChat {
bool Configure(uintptr_t gameBase);
bool Send(const std::string& utf8);
uint32_t Character(WPARAM value);
std::wstring Clipboard(HWND window);
}
