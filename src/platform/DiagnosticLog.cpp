#include "DiagnosticLog.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <share.h>

namespace {
SRWLOCK mutex = SRWLOCK_INIT;
FILE* file = nullptr;
std::string path;
LoggingSettings options;
unsigned long long bytes = 0;

void Rotate() {
    if (file) { fclose(file); file = nullptr; }
    // Previous sessions and size-limited segments use the same bounded archive chain.
    if (options.archiveCount) {
        for (unsigned index = options.archiveCount; index > 0; --index) {
            std::string target = path.substr(0, path.size() - 4) + "." + std::to_string(index) + ".log";
            std::string source = index == 1 ? path : path.substr(0, path.size() - 4) + "." + std::to_string(index - 1) + ".log";
            // Copy-overwrite also works when a live reader holds an older archive open on Windows.
            CopyFileA(source.c_str(), target.c_str(), FALSE);
        }
    }
    // Start a bounded new segment; shared readers can keep the current filename open across rotation.
    file = _fsopen(path.c_str(), "w", _SH_DENYNO);
    bytes = 0;
}
}
void DiagnosticLog::Configure(const LoggingSettings& settings) {
    AcquireSRWLockExclusive(&mutex); options = settings;
    // Reducing retention on F8 also removes surplus numbered archives, without restarting the game.
    if (!path.empty()) for (unsigned index = options.archiveCount + 1; index <= 8; ++index) {
        std::string archive = path.substr(0, path.size() - 4) + "." + std::to_string(index) + ".log";
        DeleteFileA(archive.c_str());
    }
    ReleaseSRWLockExclusive(&mutex);
}
void DiagnosticLog::Open(const char* directory) {
    AcquireSRWLockExclusive(&mutex);
    path = std::string(directory) + "\\WarcraftCS.log";
    for (unsigned index = options.archiveCount + 1; index <= 8; ++index) {
        std::string archive = path.substr(0, path.size() - 4) + "." + std::to_string(index) + ".log";
        DeleteFileA(archive.c_str());
    }
    Rotate();
    ReleaseSRWLockExclusive(&mutex);
}
void DiagnosticLog::Close() {
    AcquireSRWLockExclusive(&mutex);
    if (file) fclose(file);
    file = nullptr;
    ReleaseSRWLockExclusive(&mutex);
}
bool DiagnosticLog::Due(DWORD& previous, bool immediate) {
    AcquireSRWLockShared(&mutex); auto current = options; ReleaseSRWLockShared(&mutex);
    DWORD now = GetTickCount();
    // Diagnostic transitions can be immediate without bypassing the player's master TRACE switch.
    if (!current.detailed || (!immediate && previous && now - previous < current.intervalMs)) return false;
    previous = now; return true;
}
void DiagnosticLog::Write(const char* level, const char* format, va_list args) {
    char message[8192];
    int result = _vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);
    if (result < 0) strcpy_s(message + sizeof(message) - 16, 16, "...[truncated]");
    // Native paths use the Windows ANSI code page; persist UTF-8 for the launcher's Unicode reader.
    wchar_t wide[8192]; char utf8[32768];
    MultiByteToWideChar(CP_ACP, 0, message, -1, wide, 8192);
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof(utf8), nullptr, nullptr);
    AcquireSRWLockExclusive(&mutex);
    if (!file || (!options.detailed && !strcmp(level, "TRACE"))) { ReleaseSRWLockExclusive(&mutex); return; }
    auto limit = static_cast<unsigned long long>(options.maxFileMB) * 1024 * 1024;
    if (bytes + strlen(utf8) + 128 > limit) Rotate();
    if (bytes + strlen(utf8) + 128 > limit) {
        // A locked archive cannot grow the current log indefinitely; report the I/O problem to the debugger.
        OutputDebugStringA("WarcraftCS diagnostics: archive locked, log size limit reached.\n");
        ReleaseSRWLockExclusive(&mutex); return;
    }
    if (file) {
        SYSTEMTIME now; GetSystemTime(&now);
        // UTC timestamps plus process/thread/tick identify ordering across asynchronous hook/audio callbacks.
        int written = fprintf(file, "[%04u-%02u-%02uT%02u:%02u:%02u.%03uZ] [%lu] [%s] [pid=%lu tid=%lu] %s\n",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
            GetTickCount(), level, GetCurrentProcessId(), GetCurrentThreadId(), utf8);
        fflush(file);
        // Text mode expands LF to CRLF on Windows; use the real byte position for the storage limit.
        if (written > 0) bytes = static_cast<unsigned long long>(_ftelli64(file));
    }
    ReleaseSRWLockExclusive(&mutex);
}
