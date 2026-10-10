#include "../src/platform/NativeFogOfWar.hpp"
#include "../src/platform/WarcraftApi.hpp"
#include <cassert>
#include <cstdio>
#include <initializer_list>
namespace {
BOOL fog=TRUE,mask=TRUE;int writes=0;
void __cdecl SetFog(BOOL value) {fog=value;++writes;}
void __cdecl SetMask(BOOL value) {mask=value;++writes;}
BOOL __cdecl GetFog() {return fog;}
BOOL __cdecl GetMask() {return mask;}
}
namespace wc3 {
void (__cdecl* FogEnable)(BOOL)=SetFog;
void (__cdecl* FogMaskEnable)(BOOL)=SetMask;
BOOL (__cdecl* IsFogEnabled)()=GetFog;
BOOL (__cdecl* IsFogMaskEnabled)()=GetMask;
void Log(const char*,...) {}
}
int main() {
    // Preserve every combination of explored fog and black mask, including already-revealed maps.
    for (BOOL originalFog:{FALSE,TRUE}) for (BOOL originalMask:{FALSE,TRUE}) {
        NativeFogOfWar view;fog=originalFog;mask=originalMask;writes=0;
        view.Update(false);assert(writes==0);
        view.Update(true);assert(!fog && !mask);
        int firstWrites=writes;view.Update(true);assert(writes==firstWrites);
        // Map-script writes during the override cannot leave part of the world hidden.
        fog=mask=TRUE;view.Update(true);assert(!fog && !mask);
        view.Update(false);assert(fog==originalFog && mask==originalMask);
        firstWrites=writes;view.Update(false);assert(writes==firstWrites);
        // Returning to FPS captures fresh map preferences, rather than the previous snapshot.
        fog=!originalFog;mask=!originalMask;view.Update(true);view.Update(false);
        assert(fog==!originalFog && mask==!originalMask);
    }
    NativeFogOfWar view;fog=mask=TRUE;view.Update(true);view.Reset();
    fog=FALSE;mask=TRUE;writes=0;view.Update(false);
    assert(!fog && mask && writes==0); // Unload must never restore the previous map's fog.
    view.Update(true);view.Update(false);assert(!fog && mask);
    std::puts("PASS independent fog/mask restoration, repeat suppression, scripts and map unload");
}
