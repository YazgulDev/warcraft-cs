#pragma once
#include <windows.h>

// Enrich only faults already handled by the mod; do not intercept unrelated game exceptions.
namespace NativeFaultDiagnostics {
LONG Filter(const char* phase,EXCEPTION_POINTERS* exception);
}
