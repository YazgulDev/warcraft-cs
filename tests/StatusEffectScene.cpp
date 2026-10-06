#include "StatusEffectScene.hpp"
#include <fstream>
#include <string>

void StatusEffectScene::Tick(const ShooterController& controller, uintptr_t base, const char* root) {
    using AddAbility = BOOL (__cdecl*)(wc3::Handle, int);
    using TargetOrder = BOOL (__cdecl*)(wc3::Handle, int, wc3::Handle);
    using RemoveUnit = void (__cdecl*)(wc3::Handle);
    using SetState = void (__cdecl*)(wc3::Handle, int, float*);
    using ClearBuffs = void (__cdecl*)(wc3::Handle, BOOL, BOOL, BOOL, BOOL, BOOL, BOOL, BOOL);
    auto addAbility = reinterpret_cast<AddAbility>(base + 0x3C82A0);
    auto removeAbility = reinterpret_cast<AddAbility>(base + 0x3C8310);
    auto order = reinterpret_cast<TargetOrder>(base + 0x3C89D0);
    auto removeUnit = reinterpret_cast<RemoveUnit>(base + 0x3C8060);
    auto setState = reinterpret_cast<SetState>(base + 0x3C5EA0);
    auto clearBuffs = reinterpret_cast<ClearBuffs>(base + 0x3C8410);
    static wc3::Handle playerUnit = 0, caster = 0;
    static bool rescueByDeath = false;
    static DWORD removeAt = 0, unpauseAt = 0, nextCheck = 0, nextTrace = 0;
    wc3::Handle unit = controller.TestUnit(); DWORD now = GetTickCount();
    if (unit != playerUnit) { playerUnit = unit; caster = 0; removeAt = unpauseAt = 0; }
    if (!unit || !wc3::GetUnitTypeId(unit)) return;
    if (caster && now >= removeAt) {
        // Killing the Devour caster exercises native release; RemoveUnit is not a rescue operation.
        if (rescueByDeath) wc3::KillUnit(caster); else removeUnit(caster);
        caster = 0; rescueByDeath = false;
    }
    if (unpauseAt && now >= unpauseAt) { wc3::PauseUnit(unit, FALSE); unpauseAt = 0; }
    if (now >= nextCheck) {
        nextCheck = now + 100;
        std::string path = std::string(root) + "\\test-status.request";
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            std::ifstream file(path); std::string kind; std::getline(file, kind); file.close();
            if (!DeleteFileA(path.c_str())) return;
            if (caster) { if (rescueByDeath) wc3::KillUnit(caster); else removeUnit(caster); caster = 0; }
            rescueByDeath = false;
            // Remove previous fixture effects before the next controlled phase.
            for (int buff : {0x4253544E, 0x42656E73, 0x42656E67, 0x42656E61, 0x42736C6F}) removeAbility(unit, buff);
            // Clear even physical/non-dispellable fixture nets and stuns between phases.
            clearBuffs(unit, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE);
            float life = wc3::Real(wc3::GetUnitState(unit, 1)); setState(unit, 0, &life);
            wc3::PauseUnit(unit, FALSE); unpauseAt = 0;
            if (kind == "pause") { wc3::PauseUnit(unit, TRUE); unpauseAt = now + 3500; }
            else if (kind != "clear") {
                int type = 0, ability = 0, orderId = 0;
                // Use the actual red dragon ability and digestion cargo, never synthesize attack-disable flags.
                if (kind == "devour") { type = 0x6E72776D; ability = 0x41436476; orderId = 852247; rescueByDeath = true; }
                if (kind == "stun") { type = 0x486D6B67; ability = 0x41487462; orderId = 852095; }
                // The creep Ensnare uses the real net effect without a race-specific research requirement.
                if (kind == "root") { type = 0x6F677275; ability = 0x4143656E; orderId = 852106; }
                if (kind == "slow") { type = 0x68736F72; ability = 0x41736C6F; orderId = 852075; }
                if (type) {
                    float x = wc3::Real(wc3::GetUnitX(unit)) + (rescueByDeath ? 80 : 240), y = wc3::Real(wc3::GetUnitY(unit)), facing = 180;
                    wc3::Handle enemy = wc3::Player(12);
                    caster = wc3::CreateUnit(enemy, type, &x, &y, &facing);
                    addAbility(caster, ability); float mana = 1000; setState(caster, 2, &mana);
                    wc3::Log("status fixture %s caster=%08X accepted=%d", kind.c_str(), caster, order(caster, orderId, unit));
                    removeAt = now + (rescueByDeath ? 6000 : 1400);
                }
            }
            wc3::Log("status fixture phase=%s", kind.c_str());
        }
    }
    if (now >= nextTrace) {
        nextTrace = now + 100;
        const UnitStatus& status = controller.Status();
        wc3::Log("status oracle unit=%08X x=%.2f y=%.2f speed=%.2f stun=%d root=%d disarm=%d scale=%.3f ammo=%d weapon=%d loaded=%d devoured=%d hidden=%d hp=%.1f",
            unit, wc3::Real(wc3::GetUnitX(unit)), wc3::Real(wc3::GetUnitY(unit)), controller.MovementSpeed(),
            status.incapacitated, status.immobilized, status.disarmed, status.speedScale, controller.Ammo(), controller.WeaponIndex(), status.contained, status.devoured, status.hidden, wc3::Real(wc3::GetUnitState(unit, 0)));
    }
}
