#pragma once
#include "WarcraftApi.hpp"
#include "../config/HeroSpellSlots.hpp"
#include "../config/SpellSettings.hpp"
#include "../presentation/SpellHudSlot.hpp"

// Resolve learned skills afresh; no native ability pointer survives a frame or map transition.
class NativeSpellSlots {
public:
    void Configure(uintptr_t base);
    void Reset() {loaded_=false;slots_=HeroSpellSlots();}
    int Ability(wc3::Handle unit,int slot,const SpellSettings& settings,bool diagnostics=true);
    int Order(wc3::Handle unit,int ability);
    // Use actor-matched native command metadata instead of guessing from rawcodes or order names.
    bool PointOnly(wc3::Handle unit,int ability,int order);
    uintptr_t Object(wc3::Handle unit,int ability) const;
    SpellHudSlots Snapshot(wc3::Handle unit,const SpellSettings& settings);
private:
    std::array<int,4> Resolve(wc3::Handle unit);
    bool loaded_=false,verified_=false;
    uintptr_t base_=0;
    HeroSpellSlots slots_;
};
