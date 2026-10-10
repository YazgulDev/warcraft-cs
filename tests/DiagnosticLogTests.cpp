#include "../src/platform/DiagnosticLog.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

static void Require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
static void Record(const char* level, const char* format, ...) {
    va_list args; va_start(args, format); DiagnosticLog::Write(level, format, args); va_end(args);
}
static std::string Read(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), {});
}
int main(int argc, char** argv) {
    Require(argc == 2, "Provide an isolated log test directory");
    std::string root = argv[1]; CreateDirectoryA(root.c_str(), nullptr);
    std::string path = root + "\\WarcraftCS.log";
    LoggingSettings settings; settings.maxFileMB = 1; settings.archiveCount = 2;
    DiagnosticLog::Configure(settings); DiagnosticLog::Open(root.c_str());
    Record("INFO", "first-session"); DiagnosticLog::Close();
    DiagnosticLog::Open(root.c_str()); Record("INFO", "second-session");
    Require(Read(root + "\\WarcraftCS.1.log").find("first-session") != std::string::npos, "Previous session was lost");

    // An open shared reader must not block live writes or Windows session rotation.
    HANDLE reader = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    Require(reader != INVALID_HANDLE_VALUE, "Cannot open concurrent log reader");
    settings.detailed = false; DiagnosticLog::Configure(settings);
    Record("TRACE", "hidden-trace"); Record("ERROR", "essential-error");
    DWORD previous = 0; Require(!DiagnosticLog::Due(previous), "Detailed=false still schedules traces");
    auto text = Read(path);
    Require(text.find("hidden-trace") == std::string::npos && text.find("essential-error") != std::string::npos,
        "Essential/error versus detail filtering is incorrect");
    settings.detailed = true; settings.intervalMs = 60000; DiagnosticLog::Configure(settings);
    Require(DiagnosticLog::Due(previous) && !DiagnosticLog::Due(previous), "Trace sampling ignores configured interval");
    Record("TRACE", "live-reload-trace"); Require(Read(path).find("live-reload-trace") != std::string::npos, "Live detail reload failed");

    // Concurrent hook/audio writers must produce whole records while exceeding several file-size limits.
    std::vector<std::thread> writers;
    for (int worker = 0; worker < 4; ++worker) writers.emplace_back([worker]() {
        std::string body(400, 'x');
        for (int index = 0; index < 2000; ++index) Record("INFO", "worker=%d item=%d %s end", worker, index, body.c_str());
    });
    for (auto& writer : writers) writer.join();
    CloseHandle(reader); DiagnosticLog::Close();
    for (int index = 0; index <= 2; ++index) {
        std::string file = index == 0 ? path : root + "\\WarcraftCS." + std::to_string(index) + ".log";
        text = Read(file); Require(!text.empty() && text.size() <= 1024 * 1024, "Log size/retention limits were exceeded");
        std::istringstream lines(text); std::string line;
        while (std::getline(lines, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            Require(line.front() == '[' && line.find("[pid=") != std::string::npos && line.find(" tid=") != std::string::npos &&
                line.find("worker=") != std::string::npos && line.substr(line.size() - 4) == " end", "Concurrent records interleaved or lost metadata");
        }
    }
    Require(GetFileAttributesA((root + "\\WarcraftCS.3.log").c_str()) == INVALID_FILE_ATTRIBUTES, "Too many archives");
    // F8 retention reduction removes surplus archives; zero intentionally retains no previous segments.
    settings.archiveCount = 0; DiagnosticLog::Configure(settings);
    Require(GetFileAttributesA((root + "\\WarcraftCS.1.log").c_str()) == INVALID_FILE_ATTRIBUTES, "Retention reduction left surplus archives");
    DiagnosticLog::Open(root.c_str()); Record("INFO", "zero-archives-session"); DiagnosticLog::Close();
    Require(Read(path).find("zero-archives-session") != std::string::npos, "Zero archive mode cannot log");
    std::puts("PASS native diagnostics: session retention, shared live reader, concurrent records, size limits, live reload/filtering.");
}
