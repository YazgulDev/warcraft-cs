#pragma once
#include "../config/GameplaySettings.hpp"
#include "../platform/WarcraftApi.hpp"
#include "../platform/NativeAttackTarget.hpp"
#include <vector>

// Temporary target orders cover all owned mobile units without changing the H/O squad or its strategy.
class SquadFocusFire {
public:
    size_t Command(wc3::Handle leader,wc3::Handle target,const GameplaySettings& settings,DWORD now);
    void Tick(const GameplaySettings& settings,DWORD now);
    void Cancel();
    void Reset() {members_.clear();target_={};}
    bool Contains(uintptr_t object) const;
    bool Active() const {return target_.handle!=0;}
private:
    struct Member {wc3::Handle unit;uintptr_t object;};
    bool Valid(const Member& member) const;
    bool ValidTarget(const GameplaySettings& settings) const;
    bool Owns(const Member& member) const;
    std::vector<Member> members_;
    NativeAttackTarget target_;
    DWORD requested_=0;
};
