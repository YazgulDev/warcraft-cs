#include <windows.h>
#include <cassert>
#include <string>

using Call=int (__stdcall*)();
int main(int argc,char** argv) {
    assert(argc==3);std::string folder=argv[1];SetDllDirectoryA(folder.c_str());
    auto module=LoadLibraryA((folder+"\\Mss32.dll").c_str());assert(module);
    // The Windows loader itself resolves named/decorated/ordinal calls through the locally derived proxy.
    if(argv[2][0]=='1') {
        auto named=reinterpret_cast<Call>(GetProcAddress(module,"Named"));assert(named && named()==42);
        auto alias=reinterpret_cast<Call>(GetProcAddress(module,"Alias"));assert(alias && alias()==42);
        auto hidden=reinterpret_cast<Call>(GetProcAddress(module,MAKEINTRESOURCEA(17)));assert(hidden && hidden()==77);
        auto decorated=reinterpret_cast<Call>(GetProcAddress(module,"_Decorated@0"));assert(decorated && decorated()==99);
        assert(!GetProcAddress(module,MAKEINTRESOURCEA(12)));
    } else {
        auto other=reinterpret_cast<Call>(GetProcAddress(module,"Other"));assert(other && other()==42);
        auto hidden=reinterpret_cast<Call>(GetProcAddress(module,MAKEINTRESOURCEA(44)));assert(hidden && hidden()==77);
        assert(!GetProcAddress(module,"Named"));
    }
    assert(!GetProcAddress(module,"Missing"));FreeLibrary(module);return 0;
}
