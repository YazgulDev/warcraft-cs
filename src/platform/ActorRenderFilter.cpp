#include "ActorRenderFilter.hpp"
#include <cstring>

namespace {
uintptr_t base = 0, actorSprite = 0;
using Render = void (__fastcall*)(uintptr_t, uintptr_t);
Render originalOpaque = nullptr, originalTranslucent = nullptr;
using Submit = int (__fastcall*)(uintptr_t, uintptr_t);
using SubmitAlpha = int (__fastcall*)(uintptr_t, uintptr_t, float);
Submit originalSubmit = nullptr;
SubmitAlpha originalSubmitAlpha = nullptr;
uintptr_t Pointer(uintptr_t address) {
    uintptr_t result = 0; SIZE_T got = 0;
    if (address) ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address), &result, sizeof(result), &got);
    return result;
}
bool IsActor(uintptr_t sprite) {
    // Attached meshes follow CSprite's parent link; the complete first-person body must be excluded.
    for (unsigned depth = 0; actorSprite && sprite && depth < 32; ++depth) {
        if (sprite == actorSprite) {
            static bool logged = false;
            if (!logged) { wc3::Log("Actor world mesh excluded: sprite=%08X", actorSprite); logged = true; }
            return true;
        }
        sprite = Pointer(sprite + 0x24);
    }
    return false;
}
void __fastcall Opaque(uintptr_t sprite, uintptr_t unused) {
    if (!IsActor(sprite)) originalOpaque(sprite, unused);
}
void __fastcall Translucent(uintptr_t sprite, uintptr_t unused) {
    if (!IsActor(sprite)) originalTranslucent(sprite, unused);
}
int __fastcall SubmitActor(uintptr_t sprite, uintptr_t unused) {
    // Culling/queue submission must also omit the body, before individual material batches lose their owner.
    return IsActor(sprite) ? 0 : originalSubmit(sprite, unused);
}
int __fastcall SubmitActorAlpha(uintptr_t sprite, uintptr_t unused, float alpha) {
    return IsActor(sprite) ? 0 : originalSubmitAlpha(sprite, unused, alpha);
}
}
void ActorRenderFilter::Install(uintptr_t gameBase, Hook hook) {
    base = gameBase;
    // Both verified CSprite render paths include all material types; alpha zero alone leaves additive layers.
    const unsigned char signature[] = {0x56, 0x8B, 0xF1, 0x83, 0x7E, 0x20, 0x00};
    if (memcmp(reinterpret_cast<void*>(base + 0x4D34E0), signature, sizeof(signature)) ||
        memcmp(reinterpret_cast<void*>(base + 0x4D2E40), signature, sizeof(signature))) {
        wc3::Log("Actor sprite render signature mismatch; filter not installed"); return;
    }
    hook(reinterpret_cast<void*>(base + 0x4D34E0), reinterpret_cast<void*>(Opaque), reinterpret_cast<void**>(&originalOpaque));
    hook(reinterpret_cast<void*>(base + 0x4D2E40), reinterpret_cast<void*>(Translucent), reinterpret_cast<void**>(&originalTranslucent));
    const unsigned char submitSignature[] = {0x83,0xEC,0x60,0x56,0x8B,0xF1,0x83,0x7E,0x20,0x00};
    if (!memcmp(reinterpret_cast<void*>(base+0x4D2E60),submitSignature,sizeof(submitSignature)))
        hook(reinterpret_cast<void*>(base+0x4D2E60),reinterpret_cast<void*>(SubmitActor),reinterpret_cast<void**>(&originalSubmit));
    if (!memcmp(reinterpret_cast<void*>(base+0x4D2F10),submitSignature,sizeof(submitSignature)))
        hook(reinterpret_cast<void*>(base+0x4D2F10),reinterpret_cast<void*>(SubmitActorAlpha),reinterpret_cast<void**>(&originalSubmitAlpha));
}
void ActorRenderFilter::Begin(wc3::Handle unit) {
    // Resolve fresh each frame: a map reload or morph can replace a sprite without replacing its unit handle.
    actorSprite = 0;
    if (!unit || !base) return;
    using Resolve = uintptr_t (__fastcall*)(wc3::Handle, uintptr_t);
    uintptr_t object = reinterpret_cast<Resolve>(base + 0x3BDCB0)(unit, 0);
    if (object) actorSprite = Pointer(object + 0x28);
    static wc3::Handle loggedUnit = 0;
    if (unit != loggedUnit) {
        loggedUnit = unit;
        wc3::Log("Actor render identity unit=%08X object=%08X sprite=%08X vtable=%08X",unit,object,actorSprite,Pointer(actorSprite));
    }
}
void ActorRenderFilter::End() { actorSprite = 0; }
