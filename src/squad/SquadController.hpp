#pragma once
#include "../platform/WarcraftApi.hpp"
#include "../config/GameplaySettings.hpp"
#include "../platform/SquadSpellcasting.hpp"
#include "SquadFocusFire.hpp"
#include <vector>

// Owns the temporary FPS squad; native orders preserve pathfinding, statuses and ordinary unit attacks.
class SquadController {
public:
    void Capture(wc3::Handle leader,bool passive,const GameplaySettings& settings);
    void Tick(wc3::Handle leader,const GameplaySettings& settings,DWORD now,bool travel,float yaw,float vx,float vy);
    void Release();
    void Reset();
    size_t Focus(wc3::Handle leader,wc3::Handle target,const GameplaySettings& settings,DWORD now);
    void CancelFocus() {focus_.Cancel();}
    bool BlocksUnit(uintptr_t object) const;
    bool BlocksAttack(uintptr_t attack) const;
    size_t Count() const { return members_.size(); }
    bool Passive() const { return passive_; }
#ifdef WCS_GAMEPLAY_TEST
    void TestLog() const;
#endif
private:
    struct Member { wc3::Handle unit;uintptr_t object,attack;float targetX=0,targetY=0;DWORD orderTick=0;bool yielding=false,combatHeld=false; };
    bool Valid(const Member& member,wc3::Handle owner) const;
    std::vector<Member> members_;
    bool passive_=false;
    SquadSpellcasting spells_;
    SquadFocusFire focus_;
};
