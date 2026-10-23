#include "../src/platform/GameBuild.hpp"
#include <windows.h>
#include <cassert>
#include <cstdio>

int main(int argc,char** argv) {
    assert(argc==4);
    auto legacy=LoadLibraryExA(argv[1],nullptr,DONT_RESOLVE_DLL_REFERENCES);assert(legacy);
    assert(wc3::SelectBuild(legacy)==wc3::GameBuildKind::Classic126a);
    assert(wc3::Rva(0x3BDCB0)==0x3BDCB0 && wc3::RvaAvailable({0x395900,0x3BC4D0}));
    wc3::MarkBound();assert(wc3::IsBound());FreeLibrary(legacy);
    auto forged=LoadLibraryExA(argv[2],nullptr,DONT_RESOLVE_DLL_REFERENCES);assert(forged);
    // Metadata alone never authorizes 1.27a machine-code calls; missing prologues reject a synthetic tiny PE safely.
    assert(wc3::SelectBuild(forged)==wc3::GameBuildKind::Unsupported && !wc3::IsBound());
    assert(!wc3::Rva(0x3BDCB0));FreeLibrary(forged);
    auto unknown=LoadLibraryExA(argv[3],nullptr,DONT_RESOLVE_DLL_REFERENCES);assert(unknown);
    assert(wc3::SelectBuild(unknown)==wc3::GameBuildKind::Unsupported);FreeLibrary(unknown);
    wc3::activeBuild=wc3::GameBuildKind::Classic127a;
    assert(wc3::Rva(0x3BDCB0)==0x1D1550 && wc3::Rva(0x300710)==0x34F3A0);
    assert(wc3::Rva(0x3C5510)==0x1E66B0 && wc3::Rva(0x3C61A0)==0x1F7230);
    assert(!wc3::Rva(0x395900) && !wc3::RvaAvailable({0x3BDCB0,0x395900}));
    assert(wc3::SelectBuild(nullptr)==wc3::GameBuildKind::Unsupported);
    std::puts("PASS exact host selection, missing-signature rejection, translation and unavailable-feature isolation.");
}
