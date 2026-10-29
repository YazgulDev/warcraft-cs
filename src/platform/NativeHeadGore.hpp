#pragma once
#include "WarcraftApi.hpp"
#include <array>

// Render-only corpse deformation preserves native death events, body animation and resurrection.
namespace NativeHeadGore {
using Hook=bool (*)(void*,void*,void**);
struct Contact {
    wc3::Handle target=0;
    uintptr_t object=0,model=0;
    int type=0;
    std::array<float,3> point={};
    float weight=.35f;
    unsigned root=0xffffffff;
};
void Install(uintptr_t base,Hook hook);
Contact Prepare(wc3::Handle target,const std::array<float,3>& point,float weight,bool enabled,unsigned root);
bool Commit(const Contact& contact);
void Reset();
}
