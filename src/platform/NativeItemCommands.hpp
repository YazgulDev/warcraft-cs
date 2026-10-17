#pragma once
#include "WarcraftApi.hpp"

namespace NativeItemCommands {
// Query the same item order/target flags that Warcraft's own inventory dispatch uses.
unsigned Forms(uintptr_t base,wc3::Handle actor,int slot,wc3::Handle item);
}
