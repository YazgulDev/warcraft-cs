#pragma once
#include "NativeSpellSlots.hpp"
#include "../squad/SquadSpellPolicy.hpp"
#include "../config/GameplaySettings.hpp"
#include <unordered_map>

// Native ability discovery/order dispatch; squad following remains owned by SquadController.
class SquadSpellcasting {
public:
    bool Tick(wc3::Handle unit,const GameplaySettings& settings,DWORD now);
    void Cancel(wc3::Handle unit);
    void Forget(wc3::Handle unit) {casts_.erase(unit);} // Removed/transferred units must receive no cleanup order.
    void Reset() {casts_.clear();rules_.clear();base_=0;slots_.Reset();}
private:
    struct Cast {uintptr_t object=0;int order=0;DWORD requested=0,attempt=0;size_t cursor=0;std::unordered_map<int,DWORD> refresh;};
    void Configure();
    uintptr_t base_=0;
    NativeSpellSlots slots_;
    std::unordered_map<int,SquadSpellRole> rules_;
    std::unordered_map<wc3::Handle,Cast> casts_;
};
