#include "MapEnvironment.hpp"
#include <windows.h>
#include <cstring>

namespace MapEnvironment {
char Tileset() {
    auto storm=GetModuleHandleA("Storm.dll");if (!storm) return 0;
    auto open=reinterpret_cast<BOOL(WINAPI*)(const char*,HANDLE*)>(GetProcAddress(storm,MAKEINTRESOURCEA(267)));
    auto read=reinterpret_cast<BOOL(WINAPI*)(HANDLE,void*,DWORD,DWORD*,LONG)>(GetProcAddress(storm,MAKEINTRESOURCEA(269)));
    auto close=reinterpret_cast<BOOL(WINAPI*)(HANDLE)>(GetProcAddress(storm,MAKEINTRESOURCEA(253)));
    HANDLE file=nullptr;
    if (!open || !read || !close || !open("war3map.w3e",&file)) return 0;
    unsigned char header[9]={};DWORD got=0;
    bool ok=read(file,header,sizeof(header),&got,0) && got==sizeof(header) && !memcmp(header,"W3E!",4);
    close(file);return ok ? char(header[8]) : 0;
}
}
