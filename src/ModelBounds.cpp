#include "ModelBounds.hpp"
#include "WarcraftApi.hpp"
#include <cstring>
#include <vector>

namespace {
bool Valid(const Bounds3& b) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(b.minimum[i]) || !std::isfinite(b.maximum[i]) ||
            b.maximum[i] <= b.minimum[i] || b.maximum[i] - b.minimum[i] > 6000) return false;
    return true;
}
// Storm's open-file API searches the active map before the installed archives, including custom imports.
bool ModelBytes(const std::string& path, std::vector<unsigned char>& bytes) {
    HMODULE storm = GetModuleHandleA("Storm.dll");
    if (!storm) return false;
    auto open = reinterpret_cast<BOOL(WINAPI*)(const char*, HANDLE*)>(GetProcAddress(storm, MAKEINTRESOURCEA(267)));
    auto size = reinterpret_cast<DWORD(WINAPI*)(HANDLE, DWORD*)>(GetProcAddress(storm, MAKEINTRESOURCEA(265)));
    auto read = reinterpret_cast<BOOL(WINAPI*)(HANDLE, void*, DWORD, DWORD*, LONG)>(GetProcAddress(storm, MAKEINTRESOURCEA(269)));
    auto close = reinterpret_cast<BOOL(WINAPI*)(HANDLE)>(GetProcAddress(storm, MAKEINTRESOURCEA(253)));
    HANDLE file = nullptr;
    if (!open || !size || !read || !close || !open(path.c_str(), &file)) return false;
    DWORD length = size(file, nullptr), got = 0;
    bool ok = length >= 4 && length <= 32 * 1024 * 1024;
    if (ok) { bytes.resize(length); ok = read(file, bytes.data(), length, &got, 0) && got == length; }
    close(file); return ok;
}
bool Extents(const std::vector<unsigned char>& bytes, Bounds3& result) {
    if (bytes.size() < 4 || memcmp(bytes.data(), "MDLX", 4)) return false;
    bool found = false;
    // Stand extents exclude death/spell particles that can greatly inflate a model's global box.
    for (size_t at = 4; at + 8 <= bytes.size();) {
        unsigned length = 0; memcpy(&length, bytes.data() + at + 4, 4);
        size_t body = at + 8;
        if (length > bytes.size() - body) break;
        if (!memcmp(bytes.data() + at, "MODL", 4) && length >= 372) {
            Bounds3 b; memcpy(&b, bytes.data() + body + 344, sizeof(b));
            if (Valid(b)) { result = b; found = true; }
        }
        if (!memcmp(bytes.data() + at, "SEQS", 4)) {
            for (size_t seq = body; seq + 132 <= body + length; seq += 132) {
                if (_strnicmp(reinterpret_cast<const char*>(bytes.data() + seq), "Stand", 5)) continue;
                Bounds3 b; memcpy(&b, bytes.data() + seq + 108, sizeof(b));
                if (Valid(b)) { result = b; return true; }
            }
        }
        at = body + length;
    }
    return found;
}
}
bool ModelBounds::Load(std::string path, Bounds3& result) {
    // Warcraft object files may name MDL sources; installed archives contain the compiled MDX.
    path = path.substr(0, path.find(','));
    size_t extension = path.find_last_of('.');
    if (extension != std::string::npos) path.resize(extension);
    path += ".mdx";
    std::vector<unsigned char> bytes;
    return ModelBytes(path, bytes) && Extents(bytes, result);
}
