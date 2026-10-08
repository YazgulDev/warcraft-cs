#pragma once
#include <cstdint>
#include <string>

// Temporarily fills a missing map sky using Warcraft's own sky-model renderer.
class NativeSky {
public:
    bool Configure(uintptr_t base);
    void Update(uintptr_t ui, bool enabled, char tileset);
    void Reset();
private:
    uintptr_t base_=0,frame_=0;
    std::string model_;
    bool applied_=false;
};
