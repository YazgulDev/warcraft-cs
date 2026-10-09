#pragma once
#include "../config/LoggingSettings.hpp"
#include <windows.h>
#include <cstdarg>

// Windows adapter owns serialization, session archives and bounded diagnostic files.
class DiagnosticLog {
public:
    static void Configure(const LoggingSettings& settings);
    static void Open(const char* directory);
    static void Close();
    static void Write(const char* level, const char* format, va_list args);
    static bool Due(DWORD& previous, bool immediate = false);
};
