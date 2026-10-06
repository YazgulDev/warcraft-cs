#include "SquadController.hpp"
#include "FpsCombatGuard.hpp"
#include <algorithm>
#include <cmath>

namespace { constexpr int Stop=851972,Move=851986,Attack=851983; }
bool SquadController::Valid(const Member& member,wc3::Handle owner) const {
    // Drop removed/reused handles, ownership transfers and dead units before issuing another command.
    return wc3::UnitObject(member.unit)==member.object && wc3::GetUnitTypeId(member.unit) &&
        wc3::GetOwningPlayer(member.unit)==owner && wc3::Real(wc3::GetUnitState(member.unit,0))>.405f;
}
void SquadController::Capture(wc3::Handle leader,bool passive,const GameplaySettings& settings) {
    passive_=passive;wc3::Handle owner=wc3::GetLocalPlayer();
    members_.erase(std::remove_if(members_.begin(),members_.end(),[&](const Member& m) { return !Valid(m,owner) || m.unit==leader; }),members_.end());
    float x=wc3::Real(wc3::GetUnitX(leader)),y=wc3::Real(wc3::GetUnitY(leader)),radius=settings.squadRadius;
    auto group=wc3::CreateGroup();wc3::GroupEnumUnitsInRange(group,&x,&y,&radius,0);
    wc3::Handle unit=0;
    while ((unit=wc3::FirstOfGroup(group))!=0) {
        wc3::GroupRemoveUnit(group,unit);
        // Recruit only the player's visible mobile units; never change enemy/allied ownership or collect buildings.
        if (unit==leader || wc3::GetOwningPlayer(unit)!=owner || wc3::IsUnitType(unit,2) ||
            wc3::Real(wc3::GetUnitState(unit,0))<=.405f || wc3::IsUnitHidden(unit) || wc3::IsUnitLoaded(unit) ||
            wc3::IsUnitPaused(unit) || !wc3::IsUnitVisible(unit,owner) || wc3::Real(wc3::GetUnitMoveSpeed(unit))<=0) continue;
        if (std::any_of(members_.begin(),members_.end(),[&](const Member& m) { return m.unit==unit; })) continue;
        if (members_.size()>=size_t(settings.squadMaxUnits)) break;
        uintptr_t object=wc3::UnitObject(unit);
        if (object) members_.push_back({unit,object,*reinterpret_cast<uintptr_t*>(object+0x1E8)});
    }
    wc3::DestroyGroup(group);
    // Mode changes replace current harvest/attack orders; subsequent travel uses native move/attack-move.
    for (auto& m:members_) { wc3::IssueImmediateOrderById(m.unit,Stop);m.orderTick=0; }
    wc3::Log("squad capture mode=%s count=%u radius=%.1f",passive_ ? "follow" : "combat",unsigned(members_.size()),radius);
}
void SquadController::Tick(wc3::Handle leader,const GameplaySettings& settings,DWORD now,bool travel) {
    wc3::Handle owner=wc3::GetLocalPlayer();
    members_.erase(std::remove_if(members_.begin(),members_.end(),[&](const Member& m) { return !Valid(m,owner) || m.unit==leader; }),members_.end());
    float x=wc3::Real(wc3::GetUnitX(leader)),y=wc3::Real(wc3::GetUnitY(leader));
    for (size_t i=0;i<members_.size();++i) {
        auto& m=members_[i];m.attack=*reinterpret_cast<uintptr_t*>(m.object+0x1E8);
        if (wc3::IsUnitPaused(m.unit) || wc3::IsUnitHidden(m.unit) || wc3::IsUnitLoaded(m.unit)) continue;
        // Passive followers ignore retaliation while walking and after arrival; their combat stats stay intact.
        if (passive_) FpsCombatGuard::CancelAttacks(m.unit);
        // Transport/script hiding suspends travel to stale leader coordinates, while passive followers still avoid attacks.
        if (!travel) continue;
        if (now-m.orderTick<300) continue;
        float ux=wc3::Real(wc3::GetUnitX(m.unit)),uy=wc3::Real(wc3::GetUnitY(m.unit));
        float distance=std::hypot(ux-x,uy-y);
        float angle=float(i)*2.39996323f,ring=settings.squadFollowDistance+float(i/8)*60;
        float tx=x+std::cos(angle)*ring,ty=y+std::sin(angle)*ring;
        float targetDelta=std::hypot(tx-m.targetX,ty-m.targetY);int order=wc3::GetUnitCurrentOrder(m.unit);
        // Combat attack-move lets units acquire enemies naturally. An active fight is kept until the leash is exceeded.
        bool fighting=!passive_ && FpsCombatGuard::HasAttackTarget(m.unit);
        if (fighting && distance<settings.squadLeash) continue;
        bool outside=std::hypot(tx-ux,ty-uy)>90;
        if (outside && (!m.orderTick || targetDelta>75 || order!=(passive_ ? Move : Attack) || distance>settings.squadLeash)) {
            BOOL accepted=wc3::IssuePointOrderById(m.unit,passive_ ? Move : Attack,&tx,&ty);
            m.targetX=tx;m.targetY=ty;m.orderTick=now;
            wc3::Log("squad order unit=%08X mode=%s accepted=%d x=%.1f y=%.1f",m.unit,passive_ ? "follow" : "combat",accepted,tx,ty);
        }
    }
}
void SquadController::Release() {
    // J/F6/F10 hand followers back to Warcraft without serializing a disabled-attack state into saves.
    passive_=false;
    for (const auto& m:members_) if (Valid(m,wc3::GetLocalPlayer())) wc3::IssueImmediateOrderById(m.unit,Stop);
    Reset();
}
void SquadController::Reset() { members_.clear();passive_=false; }
bool SquadController::BlocksUnit(uintptr_t object) const {
    return passive_ && std::any_of(members_.begin(),members_.end(),[&](const Member& m) { return m.object==object; });
}
bool SquadController::BlocksAttack(uintptr_t attack) const {
    return passive_ && attack && std::any_of(members_.begin(),members_.end(),[&](const Member& m) { return m.attack==attack; });
}
#ifdef WCS_GAMEPLAY_TEST
void SquadController::TestLog() const {
    // Read-only telemetry verifies ownership filtering and native motion without altering normal gameplay.
    for (const auto& m:members_) wc3::Log("fixture squad unit=%08X owner=%08X x=%.1f y=%.1f hp=%.1f order=%d passive=%d",m.unit,
        wc3::GetOwningPlayer(m.unit),wc3::Real(wc3::GetUnitX(m.unit)),wc3::Real(wc3::GetUnitY(m.unit)),
        wc3::Real(wc3::GetUnitState(m.unit,0)),wc3::GetUnitCurrentOrder(m.unit),passive_);
}
#endif
