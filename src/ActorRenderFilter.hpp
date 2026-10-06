#pragma once
#include "WarcraftApi.hpp"

// A render-only filter: hiding a Warcraft unit through ShowUnit also changes its gameplay state.
class ActorRenderFilter {
public:
    using Hook = bool (*)(void*, void*, void**);
    static void Install(uintptr_t base, Hook hook);
    static void Begin(wc3::Handle unit);
    static void End();
};
