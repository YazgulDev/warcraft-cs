#pragma once
#include "WarcraftApi.hpp"
#include "GameplaySettings.hpp"
#include <vector>

// Owns the temporary FPS squad; native orders preserve pathfinding, statuses and ordinary unit attacks.
class SquadController {
public:
    void Capture(wc3::Handle leader,bool passive,const GameplaySettings& settings);
    void Tick(wc3::Handle leader,const GameplaySettings& settings,DWORD now,bool travel=true);
    void Release();
    void Reset();
    bool BlocksUnit(uintptr_t object) const;
    bool BlocksAttack(uintptr_t attack) const;
    size_t Count() const { return members_.size(); }
    bool Passive() const { return passive_; }
#ifdef WCS_GAMEPLAY_TEST
    void TestLog() const;
#endif
private:
    struct Member { wc3::Handle unit;uintptr_t object,attack;float targetX=0,targetY=0;DWORD orderTick=0; };
    bool Valid(const Member& member,wc3::Handle owner) const;
    std::vector<Member> members_;
    bool passive_=false;
};
