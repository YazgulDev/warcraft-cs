#pragma once
#include <cstdint>
#include <string>
#include "SkyModelLease.hpp"

// Leases a stock sky/visibility override through Warcraft's own model renderer, restoring the latest map sky.
class NativeSky {
public:
    bool Configure(uintptr_t base);
    void Update(uintptr_t ui, bool enabled, char tileset,bool hidden=false,const char* preset="auto");
    void Reset();
private:
    uintptr_t base_=0,frame_=0;
    SkyModelLease lease_;
};
