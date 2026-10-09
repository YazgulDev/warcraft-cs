#include "PlantedBomb.hpp"
#include "CombatDamage.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

bool PlantedBomb::Plant(wc3::Handle attacker, float x, float y, GameAudio& audio,float damage,float friendlyFirePercent) {
    if (timer_) return false;
    // Effects have no unit footprint or autonomous abilities, so planting cannot trap the character.
    std::string model = wc3::UnitModelPath(0x6E676C6D); // nglm: read the owner's native mine model
    if (model.empty()) return false;
    marker_ = wc3::Effect(model.c_str(), x, y);
    if (!marker_) return false;
    timer_ = wc3::CreateTimer();
    // Failed native allocation must not leave an orphan world marker or consume inventory.
    if (!timer_) { wc3::DestroyEffect(marker_);marker_=0;return false; }
    float duration = 3600; wc3::TimerStart(timer_, &duration, FALSE, 0);
    attacker_ = attacker; x_ = x; y_ = y; remaining_ = 35; nextBeep_ = 0; exploded_ = false;
    damage_=damage; // The fixed configured explosion is independent of hero attack and later reloads.
    friendlyFirePercent_=friendlyFirePercent; // Reloads affect future charges, keeping an active charge consistent.
    audio.Play("c4_plant.wav");
    wc3::Log("C4 planted marker=%08X model=%s at=%.1f,%.1f fuse=35", marker_, model.c_str(), x_, y_);
    return true;
}
bool PlantedBomb::Tick(GameAudio& audio) {
    if (!timer_) return false;
    // JASS timers follow Warcraft simulation time, so pause/Alt-Tab never skip the fuse.
    float elapsed = wc3::Real(wc3::TimerGetElapsed(timer_));
    remaining_ = std::max(0.0f, 35 - elapsed);
    if (exploded_) {
        if (elapsed >= 38) {
            wc3::DestroyTimer(timer_); Reset();
        }
        return false;
    }
    if (elapsed < 35) {
        if (elapsed >= nextBeep_) {
            const char* beep = remaining_ < 5 ? "c4_beep5.wav" : remaining_ < 10 ? "c4_beep4.wav" :
                remaining_ < 20 ? "c4_beep3.wav" : "c4_beep1.wav";
            audio.Play(beep, 0.45f);
            nextBeep_ = elapsed + (remaining_ < 5 ? 0.25f : remaining_ < 10 ? 0.5f : 1.2f);
        }
        return false;
    }
    exploded_ = true;
    audio.Play("c4_explode1.wav", 1);
    if (marker_) { wc3::DestroyEffect(marker_); marker_ = 0; }
    // Destroying this native effect plays its explosion/death sequence and releases it automatically.
    wc3::Handle explosion = wc3::Effect("Objects\\Spawnmodels\\Other\\NeutralBuildingExplosion\\NeutralBuildingExplosion.mdl", x_, y_);
    if (explosion) wc3::DestroyEffect(explosion);
    bool damaged = false;
    float radius = 800, z = wc3::Ground(x_, y_);
    wc3::Handle group = wc3::CreateGroup(), target = 0;
    std::vector<wc3::Handle> victims;
    wc3::GroupEnumUnitsInRange(group, &x_, &y_, &radius, 0);
    while ((target = wc3::FirstOfGroup(group))) {
        wc3::GroupRemoveUnit(group, target);
        if (wc3::Real(wc3::GetUnitState(target, 0)) <= 0.405f) continue;
        float dx = wc3::Real(wc3::GetUnitX(target)) - x_, dy = wc3::Real(wc3::GetUnitY(target)) - y_;
        float dz = wc3::Ground(x_ + dx, y_ + dy) + wc3::Real(wc3::GetUnitFlyHeight(target)) - z;
        if (std::sqrt(dx*dx + dy*dy + dz*dz) < radius) victims.push_back(target);
    }
    wc3::DestroyGroup(group);
    // Snapshot enumeration before damage/death triggers mutate the world; damage the planter last.
    std::stable_sort(victims.begin(), victims.end(), [this](wc3::Handle a, wc3::Handle b) {
        return a != attacker_ && b == attacker_;
    });
    for (wc3::Handle victim : victims) {
        if (!wc3::GetUnitTypeId(victim)) continue;
        float before = wc3::Real(wc3::GetUnitState(victim, 0));
        if (before <= .405f) continue;
        // The same friendly-fire rule applies to the bomb, including its planter.
        bool allied = wc3::GetUnitTypeId(attacker_) && wc3::IsUnitAlly(victim, wc3::GetOwningPlayer(attacker_));
        float damage = CombatDamage::Amount(damage_, allied, false, before,friendlyFirePercent_);
        // Chaos + universal keeps the configured damage before map triggers (default 2500).
        // Zero friendly fire must not emit a native damage event that can trigger map retaliation/reflect effects.
        if (damage>0 && wc3::GetUnitTypeId(attacker_)) wc3::UnitDamageTarget(attacker_, victim, &damage, FALSE, TRUE, 5, 26, 0);
        float after = wc3::Real(wc3::GetUnitState(victim, 0)); damaged |= after < before;
        wc3::Log("C4 blast target=%08X owner=%08X damage=%.1f hpBefore=%.1f hpAfter=%.1f", victim, wc3::GetOwningPlayer(victim), damage, before, after);
    }
    wc3::Log("C4 exploded"); return damaged;
}
void PlantedBomb::Reset() {
    // Map unload already destroyed native objects: forget handles without dereferencing them.
    attacker_ = marker_ = timer_ = 0; remaining_ = nextBeep_ = 0; exploded_ = false;
}
