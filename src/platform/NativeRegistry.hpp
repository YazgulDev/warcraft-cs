#pragma once
#include <cstdint>

// Resolve public JASS functions by their exact engine registration, never by a guessed patch RVA.
namespace NativeRegistry {
uintptr_t Resolve(uintptr_t module,const char* name,const char* signature);
}
