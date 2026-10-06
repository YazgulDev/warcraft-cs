#include "UnitStatus.hpp"
#include <algorithm>
#include <cmath>

UnitStatus UnitStatus::Read(wc3::Handle unit, uintptr_t gameBase) {
    UnitStatus result;
    // Use the same validated handle resolver as 1.26a's native unit getters, never a cached pointer.
    using ResolveUnit = uintptr_t (__fastcall*)(wc3::Handle, uintptr_t);
    uintptr_t object = reinterpret_cast<ResolveUnit>(gameBase + 0x3BDCB0)(unit, 0);
    if (!object) { result.incapacitated = result.immobilized = result.disarmed = true; return result; }
    // Bdvv is the victim's Devour vision buff; loading alone also covers ordinary transports.
    result.contained = wc3::IsUnitLoaded(unit) != FALSE;
    result.hidden = wc3::IsUnitHidden(unit) != FALSE;
    result.devoured = result.contained && (wc3::HasUnitBuff(unit, 0x42647676) || wc3::HasUnitBuff(unit, 0x42646967));
    result.stunCount = *reinterpret_cast<int*>(object + 0x198);
    result.incapacitated = result.stunCount > 0 || wc3::IsUnitPaused(unit) || result.contained || result.hidden;
    result.nativeSpeed = wc3::Real(wc3::GetUnitMoveSpeed(unit));
    result.defaultSpeed = wc3::Real(wc3::GetUnitDefaultMoveSpeed(unit));
    // Native effective speed includes stacked spells/items and returns zero while movement is disabled.
    result.immobilized = result.incapacitated || !std::isfinite(result.nativeSpeed) || result.nativeSpeed <= 0.01f;
    if (std::isfinite(result.defaultSpeed) && result.defaultSpeed > 0.01f)
        result.speedScale = std::max(0.0f, result.nativeSpeed / result.defaultSpeed);
    if (result.immobilized) result.speedScale = 0;
    // Attack-disable counters also cover effects such as hex without disabling permitted movement.
    uintptr_t attack = *reinterpret_cast<uintptr_t*>(object + 0x1E8);
    result.disarmed = result.incapacitated || (attack && *reinterpret_cast<int*>(attack + 0x40) > 0);
    return result;
}
const char* UnitStatus::Label() const {
    // Describe why the unit left combat rather than mislabelling digestion as attack disarm.
    if (devoured) return "SWALLOWED - DIGESTING";
    if (contained) return "INSIDE TRANSPORT";
    if (hidden) return "HIDDEN BY MAP";
    if (incapacitated) return "STUNNED";
    if (immobilized) return "ROOTED";
    if (disarmed) return "DISARMED";
    if (speedScale < 0.99f) return "SLOWED";
    if (speedScale > 1.01f) return "HASTED";
    return "";
}
