#include "SquadFocusFire.hpp"
#include "../platform/FpsCombatGuard.hpp"
#include <algorithm>

namespace {constexpr int Attack=851983,Stop=851972;}
bool SquadFocusFire::Valid(const Member& member) const {
    return wc3::UnitObject(member.unit)==member.object && wc3::GetUnitTypeId(member.unit) &&
        wc3::GetOwningPlayer(member.unit)==wc3::GetLocalPlayer() && wc3::Real(wc3::GetUnitState(member.unit,0))>.405f;
}
bool SquadFocusFire::ValidTarget(const GameplaySettings& settings) const {
    return target_ && targetObject_ && wc3::UnitObject(target_)==targetObject_ && wc3::GetUnitTypeId(target_) &&
        wc3::Real(wc3::GetUnitState(target_,0))>.405f && !wc3::IsUnitHidden(target_) && !wc3::IsUnitLoaded(target_) &&
        wc3::IsUnitVisible(target_,wc3::GetLocalPlayer()) &&
        (settings.targetAllies || wc3::IsUnitEnemy(target_,wc3::GetLocalPlayer()) || wc3::IsNeutralUnit(target_));
}
bool SquadFocusFire::Contains(uintptr_t object) const {
    return Active() && std::any_of(members_.begin(),members_.end(),[&](const Member& member){return member.object==object;});
}
bool SquadFocusFire::Owns(const Member& member) const {
    // A newer script/player order must survive cancellation, including attacks against a different target.
    uintptr_t current=FpsCombatGuard::AttackTargetObject(member.unit);
    return current==targetObject_ || (!current && wc3::GetUnitCurrentOrder(member.unit)==Attack);
}
size_t SquadFocusFire::Command(wc3::Handle leader,wc3::Handle target,const GameplaySettings& settings,DWORD now) {
    Cancel();target_=target;targetObject_=target ? wc3::UnitObject(target) : 0;requested_=now;
    if(!settings.squadReticleAttack || target==leader || !ValidTarget(settings)) {Reset();return 0;}
    auto group=wc3::CreateGroup();wc3::GroupEnumUnitsOfPlayer(group,wc3::GetLocalPlayer(),0);
    while(auto unit=wc3::FirstOfGroup(group)) {
        wc3::GroupRemoveUnit(group,unit);uintptr_t object=wc3::UnitObject(unit);
        // All owned mobile units participate: no H/O recruitment radius/cap, allied ownership changes or buildings.
        if(!object || unit==leader || unit==target || !Valid({unit,object}) || wc3::IsUnitType(unit,2) ||
            wc3::IsUnitPaused(unit) || wc3::IsUnitHidden(unit) || wc3::IsUnitLoaded(unit) ||
            !wc3::IsUnitVisible(unit,wc3::GetLocalPlayer()) || wc3::Real(wc3::GetUnitMoveSpeed(unit))<=0)continue;
        members_.push_back({unit,object});
    }
    wc3::DestroyGroup(group);
    // Publish ownership before dispatch so passive H/O guards permit this explicit player attack.
    members_.erase(std::remove_if(members_.begin(),members_.end(),[&](const Member& member) {
        BOOL accepted=wc3::IssueTargetOrderById(member.unit,Attack,target_);
        wc3::Log("squad focus order unit=%08X target=%08X accepted=%d",member.unit,target_,accepted);
        return !accepted;
    }),members_.end());
    if(members_.empty())Reset();return members_.size();
}
void SquadFocusFire::Tick(const GameplaySettings& settings,DWORD now) {
    // Death, fog, ownership changes and reused handles cannot leave a stale forced target behind.
    if(!settings.squadReticleAttack || !ValidTarget(settings)) {Cancel();return;}
    members_.erase(std::remove_if(members_.begin(),members_.end(),[&](const Member& member) {
        return !Valid(member) || wc3::IsUnitHidden(member.unit) || wc3::IsUnitLoaded(member.unit) ||
            (now-requested_>=100 && !Owns(member));
    }),members_.end());
    if(members_.empty())Reset();
}
void SquadFocusFire::Cancel() {
    // Empty-space clicks release only orders we still own; Warcraft acquisition and H/O then resume normally.
    for(const auto& member:members_)if(Valid(member) && Owns(member))wc3::IssueImmediateOrderById(member.unit,Stop);
    // Native health at release diagnoses whether an accepted order reached combat, without per-frame logging.
    if(Active())wc3::Log("squad focus cleared target=%08X count=%u hp=%.1f",target_,unsigned(members_.size()),
        wc3::UnitObject(target_)==targetObject_ ? wc3::Real(wc3::GetUnitState(target_,0)) : 0.f);
    Reset();
}
