#include "ShooterController.hpp"
#include "CombatDamage.hpp"
#include "MapCameraGuard.hpp"
#include "SpriteTransform.hpp"
#include "LookAngles.hpp"
#include "FpsCombatGuard.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

const Weapon ShooterController::weapons[WeaponSlots::Count] = {
    {"AK47", "ak47-1.wav", 30, 90, 36, 0.100f, 2.45f, 3000, true},
    {"M4A1", "m4a1_unsil-1.wav", 30, 90, 33, 0.0875f, 3.05f, 3000, true}, // Original CS firing interval.
    {"USP", "usp_unsil-1.wav", 12, 100, 34, 0.150f, 2.70f, 2200, false},
    {"AWP", "awp1.wav", 10, 30, 115, 1.45f, 2.90f, 4000, false},
    {"KNIFE", "knife_slash1.wav", 0, 0, 40, 0.50f, 0, 150, false},
    // C4 occupies slot 6; the sword has no ammunition and reaches farther than the knife.
    {"C4", "c4_plant.wav", 1, 0, PlantedBomb::Damage, 0, 0, 800, false},
    {"GREATSWORD", "knife_slash1.wav", 0, 0, 120, 1.84f, 0, 230, false}
};
static constexpr float radians = 0.01745329252f;
void ShooterController::Configure(const char* root, uintptr_t gameBase) {
    root_ = root;
    gameBase_ = gameBase;
    hitboxes_.Configure(gameBase);
    destructableHitboxes_.Configure(gameBase);
    audio_.Configure(root_);
    ReloadSettings();
    if (!fullscreen_.Configure(gameBase)) wc3::Log("World viewport signature mismatch");
    for (int i = 0; i < WeaponSlots::Count; ++i) { ammo_[i] = weapons[i].magazine; reserve_[i] = weapons[i].reserve; }
}
bool ShooterController::Down(int key) const { return (GetAsyncKeyState(key) & 0x8000) != 0; }
bool ShooterController::Pressed(int key) { bool down = Down(key), rising = down && !keys_[key]; keys_[key] = down; return rising; }
const char* ShooterController::Sequence(const char* name) const {
    // The exported M4/USP body is unsilenced, so pose and shot sound must agree.
    if (weapon_ == 1 || weapon_ == 2) {
        if (!strcmp(name, "shoot1")) return "shoot1_unsil";
        if (!strcmp(name, "reload")) return "reload_unsil";
        if (!strcmp(name, "draw")) return "draw_unsil";
        if (!strcmp(name, "idle")) return "idle_unsil";
    }
    if ((weapon_ == 0 || weapon_ == 3 || weapon_ == WeaponSlots::C4) && !strcmp(name, "idle")) return "idle1";
    return name;
}
float ShooterController::Play(const char* name) {
    animation_ = Sequence(name); animationTick_ = GetTickCount();
    // The supplied sword keeps its authored frame rate; only the retail knife is fitted to its cadence.
    animationRate_ = WeaponSlots::Melee(weapon_) && animation_.find("midslash") == 0 ? (weapon_ == WeaponSlots::Sword ? 1.0f : 2.5f) : 1.0f;
    return audio_.BeginAnimation(weapon_, animation_.c_str(), animationTick_);
}
void ShooterController::Sound(const char* name) {
    audio_.Play(name);
}
void ShooterController::SurfaceStep(float volume) {
    // Use CS's default concrete samples for steps, jump push-off and normal ground contact.
    static const char* steps[] = {"pl_step1.wav", "pl_step3.wav", "pl_step2.wav", "pl_step4.wav"};
    audio_.Play(steps[stepIndex_++ % 4], volume);
}
void ShooterController::SetInterface(bool show) {
    // Distinguish our HUD hiding from map-script cinematic requests observed by the native hook.
    ownInterfaceRequest_ = true;
    float zero = 0; wc3::ShowInterface(show ? TRUE : FALSE, &zero);
    ownInterfaceRequest_ = false;
}
void ShooterController::InterfaceRequest(bool show) {
    if (ownInterfaceRequest_) return;
    cinematicRequested_ = !show;
    if (active_ && !show) {
        // Yield before the map applies its cinematic layout/camera; keep the mission unit visible.
        if (unit_ && wc3::GetUnitTypeId(unit_)) wc3::SetUnitVertexColor(unit_, 255, 255, 255, 255);
        fullscreen_.Update(ui_, false); audio_.Stop(); movement_.Stop(); suspended_ = true;
        wc3::Log("FPS yielded to map cinematic");
    }
}
void ShooterController::Disable(bool restoreCamera) {
    mouseLook_.Reset();
    squad_.Release(); // Native RTS regains followers and their ordinary attack policy on F6/F10.
    // Keep the original mission unit alive and restore RTS control on exit/cutscenes.
    if (unit_ && wc3::GetUnitTypeId(unit_)) wc3::SetUnitVertexColor(unit_, 255, 255, 255, 255);
    if (restoreCamera) {
        // Restoration is our own camera write even though FPS is still active until cleanup completes.
        ownCameraRequest_ = true;
        float time = 0.25f; wc3::ResetToGameCamera(&time); SetInterface(true);
        ownCameraRequest_ = false;
    }
    CancelPlant();
    active_ = false; scopeLevel_ = 0; reloadEnd_ = 0;
    meleeContact_ = meleeReady_ = 0; recoil_.Reset();
    fullscreen_.Update(ui_, false); audio_.Stop(); stepDistance_ = 0;
    hits_.Clear(); refillRequested_ = false; refillTick_ = 0;
    itemRequested_=settingsRequested_=itemNearby_=false;squadRequested_=0;
    movement_.Stop();
    weaponWheel_.Reset(); // A partial notch must not survive leaving FPS.
    status_ = {};
    wc3::Log("FPS disabled");
}
void ShooterController::ResetMap() {
    mouseLook_.Reset();
    weaponWheel_.Reset();
    squad_.Reset(); // Forget unloaded handles without issuing commands into the next map.
    // Never dereference a unit handle after a map unload: campaign transitions reuse IDs.
    hitboxes_.Reset(); destructableHitboxes_.Reset(); bomb_.Reset(); plantProgress_ = 0;
    fixtureGate_ = 0;
    unit_ = 0; active_ = false; suspended_ = false; paused_ = false; ui_ = 0; tick_ = 0;
    scopeLevel_ = 0; cinematicRequested_ = false; toggleRequested_ = false; ownCameraRequest_ = false;
    status_ = {}; meleeContact_ = meleeReady_ = 0; recoil_.Reset();
    std::fill(std::begin(keys_), std::end(keys_), false); lastShot_ = 0;
    fullscreen_.Reset(); audio_.Stop(); stepDistance_ = 0; reloadEnd_ = 0;
    menuRequested_ = false;
    itemRequested_=settingsRequested_=itemNearby_=false;squadRequested_=0;ammoRecovery_.Reset();
    hits_.Clear(); refillRequested_ = false; refillTick_ = 0; fixtureTarget_ = 0; blastFixtures_.clear();
    for (int i = 0; i < WeaponSlots::Count; ++i) { ammo_[i] = weapons[i].magazine; reserve_[i] = weapons[i].reserve; }
}
void ShooterController::Toggle() {
    if (active_) { Disable(true); return; }
    if (!wc3::SinglePlayer()) { wc3::Log("FPS refused: requires one human player"); return; }
    unit_ = wc3::PickOwnedUnit();
    if (!unit_) { wc3::Log("FPS needs a living owned unit or hero"); return; }
    yaw_ = wc3::Real(wc3::GetUnitFacing(unit_)); pitch_ = 0;
    // F6 begins a fresh capture rather than applying the RTS cursor's distance from the center.
    mouseLook_.Reset();
    // Begin physics on the actual terrain, independent of the RTS unit's move-speed cap.
    weaponWheel_.Reset();
    movement_.Reset(wc3::Ground(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_))));
    wc3::IssueImmediateOrderById(unit_, 851972);  // stop autonomous attack/movement orders
    wc3::SetUnitVertexColor(unit_, 255, 255, 255, 0);
    float zero = 0; wc3::SetCameraTargetController(0, &zero, &zero, FALSE);
    // Save RTS anchors before the engine replaces them with cinematic letterbox bounds.
    fullscreen_.Update(ui_, true); SetInterface(false);
    active_ = true; suspended_ = false; scopeLevel_ = 0; Play("draw");
    wc3::Log("FPS enabled: unit=%08X type=%08X", unit_, wc3::GetUnitTypeId(unit_));
}
void ShooterController::AimAndMove(float dt) {
    // A contained unit belongs to native cargo: changing XY/stance would fake an escape without releasing it.
    if (status_.contained || status_.hidden) { mouseLook_.Reset();movement_.Stop(); stepDistance_ = 0; return; }
    // Mouse capture occurs only in the foreground and only after the player enables FPS.
    float dx=0,dy=0;mouseLook_.Sample(dx,dy);
    if (!status_.incapacitated) {
        float sensitivity = scopeLevel_ == 2 ? 0.009f : Scoped() ? 0.035f : 0.14f;
        LookAngles::Apply(yaw_,pitch_,dx,dy,sensitivity);
    }
    MoveInput input;
    input.forward = float(Down('W')) - float(Down('S'));
    input.right = float(Down('D')) - float(Down('A')); input.yaw = yaw_;
    input.walk = Down(VK_SHIFT); input.duck = Down(VK_CONTROL); input.jump = Down(VK_SPACE);
    input.immobilized = status_.immobilized;
    input.stanceLocked = status_.incapacitated;
    if (status_.incapacitated) input.duck = movement_.Duck() >= 0.5f;
    static const float weaponSpeeds[WeaponSlots::Count] = {221, 230, 250, 210, 250, 250, 220};
    float speed = weapon_ == 3 && Scoped() ? 150.0f : weaponSpeeds[weapon_];
    // Apply Warcraft's current modifier to CS weapon speeds; roots stop movement but still allow falling.
    speed *= status_.speedScale;
    {
        float oldX = wc3::Real(wc3::GetUnitX(unit_)), oldY = wc3::Real(wc3::GetUnitY(unit_));
        bool wasGrounded = movement_.Grounded();
        movement_.Step(input, dt, speed);
        if (status_.speedScale < 0.99f) movement_.LimitSpeed(speed * MovementPhysics::worldScale);
        bool tookOff = wasGrounded && !movement_.Grounded();
        collision_.Move(unit_, movement_, dt);
        // CS uses a surface step for moving takeoff (150 GoldSrc units/s); standing jumps are silent.
        if (tookOff && movement_.Speed() >= 150.0f * MovementPhysics::worldScale) {
            SurfaceStep(1.0f); wc3::Log("jump sound: concrete takeoff");
        }
        if ((!wasGrounded || tookOff) && movement_.Grounded()) {
            // Normal CS ground landings use the same surface family instead of pl_jump2.
            SurfaceStep(0.85f); wc3::Log("jump sound: concrete landing");
        }
        // Distance actually travelled drives steps; walls and airborne motion stay silent.
        float dxMove = wc3::Real(wc3::GetUnitX(unit_)) - oldX, dyMove = wc3::Real(wc3::GetUnitY(unit_)) - oldY;
        if (wasGrounded && movement_.Grounded()) {
            float travelled = std::sqrt(dxMove * dxMove + dyMove * dyMove);
            if (travelled > 0.01f) stepDistance_ += travelled; else stepDistance_ = 0;
            float stride = input.duck ? 75.0f : 105.0f;
            if (stepDistance_ >= stride) {
                stepDistance_ = std::fmod(stepDistance_, stride);
                SurfaceStep(input.duck ? 0.18f : input.walk ? 0.30f : 0.60f);
            }
        } else stepDistance_ = 0;
    }
    if (!status_.incapacitated) {
        float facing = LookAngles::Normalize(yaw_); wc3::SetUnitFacing(unit_, &facing);
    }
}
void ShooterController::UpdateStatus() {
    UnitStatus current = UnitStatus::Read(unit_, gameBase_);
    if (current.incapacitated != status_.incapacitated || current.immobilized != status_.immobilized ||
        current.disarmed != status_.disarmed || current.contained != status_.contained ||
        current.devoured != status_.devoured || current.hidden != status_.hidden || std::abs(current.speedScale - status_.speedScale) > 0.02f) {
        wc3::Log("unit status stun=%d root=%d disarm=%d scale=%.3f native=%.1f default=%.1f stunCount=%d loaded=%d devoured=%d hidden=%d hp=%.1f",
            current.incapacitated, current.immobilized, current.disarmed, current.speedScale,
            current.nativeSpeed, current.defaultSpeed, current.stunCount, current.contained, current.devoured, current.hidden,
            wc3::Real(wc3::GetUnitState(unit_, 0)));
        if (current.incapacitated) {
            // A stun interrupts reload/refill and cancels queued action edges instead of replaying them on recovery.
            reloadEnd_ = 0; meleeContact_ = 0; refillRequested_ = false; CancelPlant(); audio_.Stop(); Play("idle");
        }
    }
    if ((status_.contained || status_.hidden) && !current.contained && !current.hidden) {
        // On unloading/rescue, resume from the actual native position, not the stale pre-swallow eye height.
        movement_.Reset(wc3::Ground(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_))));
        wc3::SetUnitVertexColor(unit_, 255, 255, 255, 0);
    }
    if (current.contained || current.hidden) scopeLevel_ = 0;
    status_ = current;
}
void ShooterController::Camera() {
    // Keep native containment/cinematic positions intact; the overlay explains the unavailable world view.
    if (status_.contained || status_.hidden) return;
    // Presentation follows the same physical eye used by weapon rays.
    // Map RTS camera timers must not overwrite this pass; cinematics still receive control when suspended.
    ownCameraRequest_ = true;
    // Refresh alpha before rendering so scripted appearance changes cannot expose the actor around the eye.
    wc3::SetUnitVertexColor(unit_, 255, 255, 255, 0);
    camera_.Update(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_)),
        movement_.EyeZ() + wc3::Real(wc3::GetUnitFlyHeight(unit_)), yaw_ + recoil_.Yaw(), std::clamp(pitch_ + recoil_.Pitch(), -65.0f, 65.0f),
        scopeLevel_ == 2 ? 10.0f : Scoped() ? 40.0f : 85.0f);
    ownCameraRequest_ = false;
}
void ShooterController::Reload() {
    const Weapon& w = weapons[weapon_];
    if (WeaponSlots::Melee(weapon_) || weapon_ == WeaponSlots::C4 || reloadEnd_ || ammo_[weapon_] == w.magazine || reserve_[weapon_] <= 0) return;
    // Complete the reload with the model; magazine/bolt sounds are its frame events.
    float duration = Play("reload");
    recoil_.ResetBurst(); // Native CS reload resets burst accuracy, without snapping the camera punch away.
    reloadEnd_ = animationTick_ + DWORD((duration > 0 ? duration : w.reload) * 1000);
}
void ShooterController::RefillAmmo() {
    // Refill every carried weapon together, including reserves, and cancel an unfinished reload.
    for (int i = 0; i < WeaponSlots::Count; ++i) { ammo_[i] = weapons[i].magazine; reserve_[i] = weapons[i].reserve; }
    if (reloadEnd_) { reloadEnd_ = 0; audio_.CancelAnimation(); Play("idle"); }
    CancelPlant();
    refillTick_ = GetTickCount();
    ammoRecovery_.Reset();ammoMessage_="ALL AMMO RESTORED";
    wc3::Log("F7 ammo refill: AK47=30/90 M4A1=30/90 USP=12/100 AWP=10/30 C4=1");
}
void ShooterController::SwitchWeapon(int slot) {
    if (weapon_ == slot) return;
    // Number keys and the wheel share deployment: cancel reload, scope, planting and pending melee cues.
    CancelPlant(); audio_.CancelAnimation(); meleeContact_ = meleeReady_ = 0;
    recoil_.ResetBurst(); weapon_ = slot; reloadEnd_ = 0; scopeLevel_ = 0; Play("draw");
    wc3::Log("weapon selected=%s slot=%d", weapons[slot].name, slot + 1);
}
void ShooterController::ReloadSettings() {
    // Replace one complete snapshot; stale fractional credit must not survive a percentage/scope change.
    settings_=GameplaySettings::Load(root_+"\\WarcraftCS.ini");
    ammoRecovery_.Reset();ammoMessage_="SETTINGS RELOADED";refillTick_=GetTickCount();
    wc3::Log("Settings loaded runePercent=%.1f ammoWeapons=%s damageMode=%s awpOneShot=%d radius=%.1f",
        settings_.runeAmmoPercent,settings_.runeAmmoAllWeapons ? "all" : "current",
        settings_.heroDamage ? "hero" : "weapon",settings_.awpOneShot,settings_.runePickupRadius);
}
void ShooterController::PickupItem() {
    if (status_.incapacitated || status_.contained || status_.hidden) return;
    ItemPickupResult result=itemPickup_.Take(unit_,settings_.runePickupRadius);
    if (!result.found) return;
    // Equipment uses native inventory slots; only successful powerups award the configured rune ammo.
    if (!result.accepted || !result.powerup) {
        ammoMessage_=result.accepted ? "ITEM PICKED UP" : "CANNOT PICK UP ITEM";
        refillTick_=GetTickCount();return;
    }
    // Refill configured magazines and reserves without exceeding their capacities; C4 uses fractional credit.
    int added=0;
    for (int slot=0;slot<WeaponSlots::Count;++slot) {
        if (!settings_.runeAmmoAllWeapons && slot!=weapon_) continue;
        added+=ammoRecovery_.Restore(slot,settings_.runeAmmoPercent,weapons[slot].magazine,weapons[slot].reserve,ammo_[slot],reserve_[slot]);
        wc3::Log("rune ammo weapon=%s ammo=%d reserve=%d",weapons[slot].name,ammo_[slot],reserve_[slot]);
    }
    char message[96];sprintf_s(message,"RUNE: +%.0f%% %s AMMO",settings_.runeAmmoPercent,settings_.runeAmmoAllWeapons ? "ALL" : "CURRENT");
    ammoMessage_=message;refillTick_=GetTickCount();itemNearby_=false;
    wc3::Log("rune ammo reward added=%d",added);
}
void ShooterController::Fire(bool secondary) {
    const Weapon& w = weapons[weapon_]; DWORD now = GetTickCount();
    // Keep CS's fractional firing interval: truncating 87.5ms would permit an early M4 shot.
    if (reloadEnd_ || now < meleeReady_ || float(now - lastShot_) < w.interval * 1000) return;
    if (!WeaponSlots::Melee(weapon_) && ammo_[weapon_] <= 0) { Reload(); return; }
    lastShot_ = now; if (!WeaponSlots::Melee(weapon_)) --ammo_[weapon_];
    wc3::Log("fire weapon=%s ammo=%d mode=%s", w.name, ammo_[weapon_], secondary ? "secondary" : "primary");
    // slash1/slash2 in this retail knife contain only two frames; mid-slashes contain the swing.
    // Secondary melee uses each model's authored stab, sharing cooldown with primary slashes.
    const char* action = WeaponSlots::Melee(weapon_) ? (secondary ? "stab" : (knifeSwing_++ % 2 ? "midslash2" : "midslash1")) : "shoot1";
    float duration = Play(action); Sound(w.sound);
    if (WeaponSlots::Melee(weapon_)) {
        // Delay damage and the contact sample until the blade reaches the target, never at button-down.
        meleeAttack_ = MeleeAttack::Select(weapon_, secondary);
        meleeContact_ = now + DWORD(meleeAttack_.contactSeconds * 1000);
        meleeReady_ = now + DWORD(std::max(meleeAttack_.recoverySeconds, duration / animationRate_) * 1000);
        return;
    }
    // GoldSrc fires with the existing punch angle, then applies the next kick to camera and later bullets.
    Strike();
    recoil_.Shot(weapon_, movement_.Speed(), movement_.Grounded(), movement_.Duck() >= .5f);
    // Record the sampled pose with the angular kick so native stance tests can distinguish input from terrain.
    wc3::Log("recoil weapon=%s pitch=%.3f yaw=%.3f shots=%u speed=%.3f grounded=%d duck=%.2f", w.name,
        recoil_.Pitch(), recoil_.Yaw(), recoil_.Shots(),movement_.Speed(),movement_.Grounded(),movement_.Duck());
}
void ShooterController::Strike() {
    const Weapon& w = weapons[weapon_];

    float x = wc3::Real(wc3::GetUnitX(unit_)), y = wc3::Real(wc3::GetUnitY(unit_));
    // Shots originate at the same eye used by the camera, including airborne/crouched poses.
    float z = movement_.EyeZ() + wc3::Real(wc3::GetUnitFlyHeight(unit_));
    float yaw = (yaw_ + recoil_.Yaw()) * radians, pitch = std::clamp(pitch_ + recoil_.Pitch(), -65.0f, 65.0f) * radians;
    float vx = std::cos(pitch) * std::cos(yaw), vy = std::cos(pitch) * std::sin(yaw), vz = std::sin(pitch);
    float attackRange = WeaponSlots::Melee(weapon_) ? meleeAttack_.range : w.range;
    float nearest = attackRange;
    // Terrain stops the ray before units behind a hill can be damaged.
    for (float t = 48; t < nearest; t += 48) {
        if (wc3::Ground(x + vx * t, y + vy * t) > z + vz * t) { nearest = t; break; }
    }
    wc3::Handle group = wc3::CreateGroup(), target = 0, candidate = 0, local = wc3::GetLocalPlayer();
    // Include models whose surface enters range before their center.
    float radius = attackRange + 512; wc3::GroupEnumUnitsInRange(group, &x, &y, &radius, 0);
    while ((candidate = wc3::FirstOfGroup(group)) != 0) {
        wc3::GroupRemoveUnit(group, candidate);
        // Friendly fire applies to every other visible living unit/building, regardless of ownership.
        if (candidate == unit_ || wc3::Real(wc3::GetUnitState(candidate, 0)) <= 0.405f ||
            !wc3::IsUnitVisible(candidate, local)) continue;
        // Intersect the rendered model's oriented 3D volume instead of one fixed ground cylinder.
        float origin[] = {x, y, z}, direction[] = {vx, vy, vz}, entry = 0;
        float padding = weapon_ == WeaponSlots::Sword ? 28.0f : 0;
        if (hitboxes_.Intersect(candidate, origin, direction, nearest, entry, padding)) {
            nearest = entry; target = candidate;
        }
    }
    wc3::DestroyGroup(group);
    // Destructables are widgets, not units: gates must compete with unit surfaces for the nearest ray hit.
    bool destructable = false;
    float origin[] = {x,y,z}, direction[] = {vx,vy,vz};
    for (wc3::Handle object : wc3::NearbyDestructables(x,y,radius)) {
        if (wc3::Real(wc3::GetDestructableLife(object)) <= .405f) continue;
        float entry=0;
        if (destructableHitboxes_.Intersect(object,origin,direction,nearest,entry)) {
            nearest=entry; target=object; destructable=true;
        }
    }
    if (target) {
        // The existing hero is the attacker, preserving mission kill/XP trigger ownership.
        auto life = [&]() { return wc3::Real(destructable ? wc3::GetDestructableLife(target) : wc3::GetUnitState(target,0)); };
        float before = life();
        bool allied = !destructable && wc3::IsUnitAlly(target,wc3::GetOwningPlayer(unit_));
        bool lethal = !destructable && CombatDamage::FinishesTarget(settings_.FinishingAWP(weapon_),allied);
        // Alliance changes are evaluated at contact; friendly AWP uses 50% of its ordinary 115 damage.
        // Hero mode applies the current attack and per-weapon multiplier at contact; friendly reduction follows it.
        float attack=settings_.heroDamage ? wc3::UnitAttackAverage(unit_) : 0;
        float baseDamage=settings_.ContactDamage(weapon_,WeaponSlots::Melee(weapon_) && meleeAttack_.secondary,attack);
        float damage = CombatDamage::Amount(baseDamage,allied,lethal,before);
        if (settings_.heroDamage) wc3::Log("hero damage attack=%.1f multiplier=%.2f base=%.1f",attack,settings_.heroMultiplier[weapon_],baseDamage);
        // Universal AWP damage ignores Warcraft armor; native death keeps death triggers intact.
        wc3::UnitDamageTarget(unit_, target, &damage, TRUE, TRUE, 0, lethal ? 26 : 4, 0);
        if (lethal && wc3::Real(wc3::GetUnitState(target, 0)) > 0.405f) wc3::KillUnit(target);
        float after = life();
        if (after < before) {
            // The same hit marker confirms damage to every faction, without blood.
            hits_.Record();
            if (WeaponSlots::Melee(weapon_)) {
                static const char* contacts[] = {"knife_hit1.wav", "knife_hit2.wav", "knife_hit3.wav", "knife_hit4.wav"};
                const char* sample = destructable ? "knife_hitwall1.wav" : (weapon_ == WeaponSlots::Sword || meleeAttack_.secondary) ? "knife_stab.wav" : contacts[knifeSwing_ % 4];
                Sound(sample); wc3::Log("melee contact weapon=%s sound=%s animationTime=%.3f mode=%s", w.name, sample, AnimationTime(), meleeAttack_.secondary ? "secondary" : "primary");
            }
            wc3::Log("hit feedback=marker");
        }
        wc3::Log("shot weapon=%s target=%08X damage=%.1f hpBefore=%.1f hpAfter=%.1f allied=%d destructable=%d", w.name, target, damage, before, after,allied,destructable);
    } else if (WeaponSlots::Melee(weapon_) && nearest < attackRange) {
        // Terrain contact uses the original hard-surface sample; misses keep only the swing sound.
        Sound("knife_hitwall1.wav");
    }
}
void ShooterController::CancelPlant() {
    if (plantProgress_ <= 0) return;
    plantProgress_ = 0; audio_.CancelAnimation(); Play("idle");
    wc3::Log("C4 planting canceled");
}
void ShooterController::PlantC4(float dt) {
    float x = wc3::Real(wc3::GetUnitX(unit_)), y = wc3::Real(wc3::GetUnitY(unit_));
    // Releasing fire, moving, jumping or losing attack control cancels an unfinished installation.
    bool moving = Down('W') || Down('A') || Down('S') || Down('D') || Down(VK_SPACE);
    if (!Down(VK_LBUTTON) || moving || !movement_.Grounded() || status_.disarmed ||
        ammo_[WeaponSlots::C4] <= 0 || bomb_.Active()) { CancelPlant(); return; }
    if (plantProgress_ == 0) { plantX_ = x; plantY_ = y; Play("pressbutton"); }
    if (std::abs(x - plantX_) + std::abs(y - plantY_) > 4) { CancelPlant(); return; }
    plantProgress_ += dt;
    if (plantProgress_ >= 3) {
        if (bomb_.Plant(unit_, x, y, audio_,settings_.damage[WeaponSlots::C4])) { --ammo_[WeaponSlots::C4]; Play("drop"); }
        plantProgress_ = 0;
    }
}
const char* ShooterController::Animation() const { return animation_.c_str(); }
float ShooterController::AnimationTime() const { return (GetTickCount() - animationTick_) * 0.001f * animationRate_; }
void ShooterController::Tick(uintptr_t ui) {
    if (ui_ && ui != ui_) ResetMap(); ui_ = ui;
    DWORD now = GetTickCount(); float dt = tick_ ? std::min((now - tick_) * 0.001f, 0.05f) : 0; tick_ = now;
    DWORD foregroundPid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
    if (bomb_.Tick(audio_) && Visible()) hits_.Record();
    if (foregroundPid != GetCurrentProcessId()) {
        // Drop background packets; refocusing establishes a fresh cursor anchor without a view jump.
        mouseLook_.Reset();
        weaponWheel_.Reset();
        itemRequested_=false;squadRequested_=0; // Commands pressed before losing focus must not execute on return.
        CancelPlant(); meleeContact_ = 0;
        // Cancel queued animation events rather than releasing delayed reload sounds on return.
        audio_.CancelAnimation(); return;
    }
    // ShowInterface(false) disables Warcraft input itself; FPS supplies its own controls.
    // Map-script interface requests are distinct from the FPS mode's own hidden HUD.
    bool cinematic = cinematicRequested_;
    // Native menus have their own pause flags, separate from the JASS PauseGame callback.
    bool suspended = cinematic || paused_ || *reinterpret_cast<int*>(ui + 0x258) || *reinterpret_cast<int*>(ui + 0x260);
    if (active_ && suspended != suspended_) {
        // Yield actor visibility and layout to native menus/cinematics; mission bounds stay untouched.
        // Map scripts can remove the controlled unit before their next cinematic callback.
        if (unit_ && wc3::GetUnitTypeId(unit_)) wc3::SetUnitVertexColor(unit_, 255, 255, 255, suspended ? 255 : 0);
        if (!suspended) fullscreen_.Update(ui_, true);
        if (!cinematic) SetInterface(suspended);
        audio_.Stop(); stepDistance_ = 0;
        wc3::Log("FPS %s for campaign/UI", suspended ? "suspended" : "resumed");
    }
    suspended_ = suspended;
    if (suspended_) { CancelPlant(); meleeContact_ = 0; }
    // Restore Warcraft input before forwarding F10, which cinematic UI otherwise ignores.
    if (menuRequested_) {
        menuRequested_ = false; Disable(true);
        HWND window = GetForegroundWindow();
        PostMessageA(window, WM_SYSKEYDOWN, VK_F10, 1 | (0x44 << 16));
        PostMessageA(window, WM_SYSKEYUP, VK_F10, 1 | (0x44 << 16) | (1L << 30) | (1L << 31));
        return;
    }
    // Window messages retain short key taps that async polling can miss between frames.
    if (toggleRequested_) {
        // A deliberate F6 can enter custom maps whose scripts keep the standard HUD hidden.
        if (!active_ && !paused_ && !*reinterpret_cast<int*>(ui + 0x258) && !*reinterpret_cast<int*>(ui + 0x260)) {
            cinematicRequested_ = false; suspended_ = false; Toggle();
        } else if (active_) Toggle();
    }
    toggleRequested_ = false;
    fullscreen_.Update(ui_, Visible());
    if (!active_ || suspended_) { mouseLook_.Reset();weaponWheel_.Reset();itemRequested_=false;squadRequested_=0;return; }
    if (settingsRequested_) { settingsRequested_=false;ReloadSettings(); }
    // An explicit local test request creates one stationary target for damage verification.
    // Normal launches never create units; the request is consumed once on the game thread.
    static DWORD lastRequestCheck = 0;
    if (now - lastRequestCheck > 500) {
        lastRequestCheck = now;
        std::string request = root_ + "\\test-target.request";
        if (GetFileAttributesA(request.c_str()) != INVALID_FILE_ATTRIBUTES) {
            // Local fixture labels exercise different target types and melee reach.
            std::ifstream file(request); std::string fixture; std::getline(file, fixture); file.close();
            if (!DeleteFileA(request.c_str())) return;
            if (fixtureTarget_ && wc3::GetUnitTypeId(fixtureTarget_)) wc3::RemoveUnit(fixtureTarget_);
            fixtureTarget_ = 0;
            if (fixtureGate_ && wc3::GetDestructableTypeId(fixtureGate_)) wc3::RemoveDestructable(fixtureGate_);
            fixtureGate_=0;
            for (wc3::Handle fixtureUnit : blastFixtures_) if (wc3::GetUnitTypeId(fixtureUnit)) wc3::RemoveUnit(fixtureUnit);
            blastFixtures_.clear();
            if (fixture == "inspect") {
                // Read-only local diagnosis compares rendered transforms with logical widget coordinates.
                float x=wc3::Real(wc3::GetUnitX(unit_)), y=wc3::Real(wc3::GetUnitY(unit_));
                for (wc3::Handle objectHandle : wc3::NearbyDestructables(x,y,1600)) {
                    int type=wc3::GetDestructableTypeId(objectHandle);
                    if (type!=0x4C546531 && type!=0x4C546732 && type!=0x4C546733 && type!=0x4C546734) continue;
                    using Resolve=uintptr_t (__fastcall*)(wc3::Handle,uintptr_t);
                    uintptr_t object=reinterpret_cast<Resolve>(gameBase_+0x3BE010)(objectHandle,0);
                    uintptr_t sprite=*reinterpret_cast<uintptr_t*>(object+0x28);
                    ModelTransform transform;
                    if (!SpriteTransform::Read(sprite,transform)) continue;
                    const float* m=transform.matrix;
                    wc3::Log("inspect gate=%08X type=%08X hp=%.1f pos=%.1f,%.1f,%.1f matrix=%.3f,%.3f,%.3f/%.3f,%.3f,%.3f/%.3f,%.3f,%.3f",objectHandle,type,wc3::Real(wc3::GetDestructableLife(objectHandle)),transform.position[0],transform.position[1],transform.position[2],m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8]);
                }
            } else if (fixture.find("aim ")==0) {
                // A consumed local aim request verifies real map surfaces without spawning or relocating anything.
                std::istringstream input(fixture);std::string command;float x=0,y=0,z=0;
                if (input>>command>>x>>y>>z) {
                    float dx=x-wc3::Real(wc3::GetUnitX(unit_)),dy=y-wc3::Real(wc3::GetUnitY(unit_));
                    yaw_=std::atan2(dy,dx)/radians;
                    pitch_=std::atan2(z-movement_.EyeZ()-wc3::Real(wc3::GetUnitFlyHeight(unit_)),std::hypot(dx,dy))/radians;
                    recoil_.Reset();
                    wc3::Log("verification aim=%.1f,%.1f,%.1f yaw=%.2f pitch=%.2f",x,y,z,yaw_,pitch_);
                }
            } else if (fixture.find("gate ")==0) {
                // Explicit gate fixtures verify native widget damage, orientation and closest-object occlusion.
                std::istringstream input(fixture); std::string command,raw;
                float distance=300,facing=0,scale=1; input>>command>>raw>>distance>>facing;
                // Optional authored scale exercises gates wider/taller than the stock one-unit fixture.
                input>>scale;
                if (!std::isfinite(scale)||scale<.1f||scale>10) scale=1;
                int type=0x4C546731;
                if (raw.size()==4) type=(raw[0]<<24)|(raw[1]<<16)|(raw[2]<<8)|raw[3];
                float x=wc3::Real(wc3::GetUnitX(unit_))+distance*std::cos(yaw_*radians);
                float y=wc3::Real(wc3::GetUnitY(unit_))+distance*std::sin(yaw_*radians);
                fixtureGate_=wc3::CreateDestructable(type,&x,&y,&facing,&scale,0);
                pitch_=std::atan2(wc3::Ground(x,y)+120-movement_.EyeZ(),distance)/radians;
                wc3::Log("verification gate=%08X type=%08X facing=%.1f nativeFacing=%.3f scale=%.2f hp=%.1f",fixtureGate_,type,facing,wc3::DestructableFacing(fixtureGate_),scale,wc3::Real(wc3::GetDestructableLife(fixtureGate_)));
            } else if (fixture == "blast" || fixture == "blast2500" || fixture == "blast50") {
                // This consumed local request tests friendly fire and the radius boundary without touching normal maps.
                const int owners[] = {12, 15, 0, 12};
                const int types[] = {0x686B6E69, 0x68666F6F, 0x68746F77, 0x686B6E69};
                // The fixed-damage oracle includes a natural 5000HP unit and a small friendly building.
                const int damageTypes[] = {0x4E6D616E, 0x68666F6F, 0x68686F75, 0x68666F6F};
                const float distances[] = {300, 450, 650, 950};
                for (int i = 0; i < 4; ++i) {
                    float x = wc3::Real(wc3::GetUnitX(unit_)) + distances[i] * std::cos(yaw_ * radians);
                    float y = wc3::Real(wc3::GetUnitY(unit_)) + distances[i] * std::sin(yaw_ * radians);
                    float facing = 0;
                    wc3::Handle owner = i == 2 || (fixture=="blast50" && i==1) ? wc3::GetLocalPlayer() : wc3::Player(owners[i]);
                    int type=fixture=="blast50" && i==1 ? 0x4E6D616E : fixture != "blast" ? damageTypes[i] : types[i];
                    wc3::Handle created = wc3::CreateUnit(owner, type, &x, &y, &facing);
                    wc3::PauseUnit(created, TRUE); blastFixtures_.push_back(created);
                    wc3::Log("blast fixture index=%d unit=%08X owner=%08X distance=%.0f x=%.1f y=%.1f hp=%.1f", i, created, owner, distances[i],
                        wc3::Real(wc3::GetUnitX(created)), wc3::Real(wc3::GetUnitY(created)), wc3::Real(wc3::GetUnitState(created, 0)));
                }
            } else if (fixture != "clear") {
                bool undeadFixture = fixture.find("undead") != std::string::npos;
                int type = undeadFixture ? 0x7567686F : 0x68666F6F;
                float distance = fixture.find("close") != std::string::npos ? 100.0f : 250.0f;
                float side = 0, aimHeight = 72, scale = 1; int fixtureOwner = 12;
                // An explicit rawcode fixture reproduces tall/mounted/flying edge hits without changing ordinary maps.
                if (fixture.find("type ") == 0) {
                    std::istringstream input(fixture); std::string command, raw;
                    input >> command >> raw >> distance >> side >> aimHeight >> scale;
                    // Optional owner slot verifies passive neutral targets without changing normal gameplay.
                    int requestedOwner = 12;
                    if (input >> requestedOwner) fixtureOwner = std::clamp(requestedOwner, 0, 15);
                    if (raw.size() == 4) type = (raw[0]<<24) | (raw[1]<<16) | (raw[2]<<8) | raw[3];
                }
                float c = std::cos(yaw_ * radians), s = std::sin(yaw_ * radians);
                float x = wc3::Real(wc3::GetUnitX(unit_)) + distance*c - side*s;
                float y = wc3::Real(wc3::GetUnitY(unit_)) + distance*s + side*c;
                float facing = yaw_ + 180;
                fixtureTarget_ = wc3::CreateUnit(wc3::Player(fixtureOwner), type, &x, &y, &facing);
                wc3::PauseUnit(fixtureTarget_, TRUE);
                if (scale != 1) wc3::SetUnitScale(fixtureTarget_, &scale, &scale, &scale);
                if (fixture.find("type ") == 0) {
                    // Warcraft snaps/repositions created buildings; aim a centered fixture at its actual location.
                    if (side == 0) {
                        x = wc3::Real(wc3::GetUnitX(fixtureTarget_)); y = wc3::Real(wc3::GetUnitY(fixtureTarget_));
                        float dx = x - wc3::Real(wc3::GetUnitX(unit_)), dy = y - wc3::Real(wc3::GetUnitY(unit_));
                        distance = std::hypot(dx, dy); yaw_ = std::atan2(dy, dx) / radians;
                    }
                    float z = wc3::Ground(x, y) + wc3::Real(wc3::GetUnitFlyHeight(fixtureTarget_)) + aimHeight;
                    pitch_ = std::atan2(z - movement_.EyeZ(), distance) / radians;
                }
                wc3::Log("verification target=%08X type=%08X at=%.1f,%.1f fly=%.1f side=%.1f height=%.1f scale=%.1f ownerSlot=%d neutral=%d",
                    fixtureTarget_, type, x, y, wc3::Real(wc3::GetUnitFlyHeight(fixtureTarget_)), side, aimHeight, scale, fixtureOwner, wc3::IsNeutralUnit(fixtureTarget_));
            }
        }
    }
    if (Pressed(VK_ESCAPE) || !wc3::GetUnitTypeId(unit_) || wc3::Real(wc3::GetUnitState(unit_, 0)) <= 0.405f) { Disable(true); return; }
    health_ = wc3::Real(wc3::GetUnitState(unit_, 0));
    UpdateStatus();
    // J gives followers back to native control even when their FPS leader cannot act.
    if (squadRequested_==3) {
        squad_.Release();squadRequested_=0;ammoMessage_="SQUAD RELEASED";refillTick_=now;
        wc3::Log("squad released by J: count=%u",unsigned(squad_.Count()));
    }
    recoil_.Step(dt, Down(VK_LBUTTON) && !status_.incapacitated && !status_.disarmed, weapon_);
    if (status_.incapacitated) {
        // Discard scrolling during disabled states rather than replaying a switch after recovery.
        weaponWheel_.Reset();
        // Disabled leaders cannot queue pickup/recruitment for later; existing followers retain their chosen policy.
        itemRequested_=false;squadRequested_=0;
        squad_.Tick(unit_,settings_,now,!status_.contained && !status_.hidden);
        // Consume action edges while disabled, so held reload/scope/switch inputs are not replayed on recovery.
        static const int actionKeys[] = {'1', '2', '3', '4', '5', '6', '7', 'R', VK_LBUTTON, VK_RBUTTON};
        for (int key : actionKeys) Pressed(key);
        refillRequested_ = false;
        AimAndMove(dt); Camera(); audio_.Tick(GetTickCount());
        return;
    }
    if (refillRequested_) { refillRequested_ = false; RefillAmmo(); }
    // Wheel packets are consumed once; an explicit number key takes precedence in the same frame.
    SwitchWeapon(weaponWheel_.Take(weapon_));
    for (int i = 0; i < WeaponSlots::Count; ++i) if (Pressed('1' + i)) SwitchWeapon(i);
    if (Pressed('R')) Reload();
    // Consume the right-click edge for every weapon so holding it across a switch never scopes or stabs.
    bool secondaryPressed = Pressed(VK_RBUTTON);
    if (weapon_ == 3 && secondaryPressed) {
        // CS-style right-click cycle: normal view, first zoom, second zoom, normal view.
        scopeLevel_ = (scopeLevel_ + 1) % 3;
        wc3::Log("AWP scope level=%d fov=%.0f", scopeLevel_, scopeLevel_ == 2 ? 10.0f : Scoped() ? 40.0f : 85.0f);
    }
    if (reloadEnd_ && now >= reloadEnd_) {
        int amount = std::min(weapons[weapon_].magazine - ammo_[weapon_], reserve_[weapon_]);
        ammo_[weapon_] += amount; reserve_[weapon_] -= amount; reloadEnd_ = 0; Play("idle");
        wc3::Log("reload complete: weapon=%s ammo=%d reserve=%d", weapons[weapon_].name, ammo_[weapon_], reserve_[weapon_]);
    }
    // Native AI attacks/orders must not compete with FPS movement or manually fired damage.
    if (!status_.incapacitated && !status_.contained && !status_.hidden) FpsCombatGuard::CancelOrders(unit_);
    if (itemRequested_) { itemRequested_=false;PickupItem(); }
    if (squadRequested_) {
        // Both keys recruit nearby owned units and change the existing squad's strategy as one operation.
        if (!status_.incapacitated && !status_.contained && !status_.hidden) {
            squad_.Capture(unit_,squadRequested_==2,settings_);
            char message[96];sprintf_s(message,"SQUAD: %u | %s",unsigned(squad_.Count()),squad_.Passive() ? "FOLLOW ONLY" : "FOLLOW + FIGHT");
            ammoMessage_=message;refillTick_=now;
        }
        squadRequested_=0;
    }
    squad_.Tick(unit_,settings_,now,!status_.contained && !status_.hidden);
    static DWORD lastItemScan=0;
    if (now-lastItemScan>200) {
        lastItemScan=now;itemNearby_=!status_.incapacitated && !status_.contained && !status_.hidden && itemPickup_.Nearest(unit_,settings_.runePickupRadius)!=0;
    }
    // Pending melee contact is cancelled if the unit loses attack control before the blade lands.
    if (meleeContact_ && (status_.disarmed || status_.incapacitated)) meleeContact_ = 0;
    if (meleeContact_ && now >= meleeContact_) { meleeContact_ = 0; Strike(); }
    bool firePressed = Pressed(VK_LBUTTON);
    // CS chooses recoil after the current movement command, so the first moving/ducked shot uses that stance.
    AimAndMove(dt);
    // Planting requires a held button; other weapons preserve their existing click/automatic behavior.
    if (weapon_ == WeaponSlots::C4) PlantC4(dt);
    else if (!status_.disarmed && WeaponSlots::Melee(weapon_) && secondaryPressed) Fire(true);
    else if (!status_.disarmed && ((weapons[weapon_].automatic && Down(VK_LBUTTON)) || firePressed)) Fire();
    // Sample after any newly started animation: an older tick would wrap unsigned elapsed time.
    Camera(); audio_.Tick(GetTickCount());
    // A low-frequency state trace makes cliff clearance, jump and stance tests observable.
    static DWORD lastState = 0;
    if (now - lastState > 1000) {
        lastState = now;
        wc3::Log("movement x=%.1f y=%.1f ground=%.1f feet=%.1f eye=%.1f speed=%.1f duck=%.2f grounded=%d cameraEye=%.1f cameraX=%.1f cameraY=%.1f pitch=%.1f distance=%.1f fly=%.1f yaw=%.3f cameraYaw=%.3f rawMouse=%d",
            wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_)),
            wc3::Ground(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_))),
            movement_.FeetZ(), movement_.EyeZ(), movement_.Speed(), movement_.Duck(), movement_.Grounded(),
            wc3::Real(wc3::GetCameraEyePositionZ()), wc3::Real(wc3::GetCameraEyePositionX()),
            wc3::Real(wc3::GetCameraEyePositionY()), pitch_, wc3::Real(wc3::GetCameraField(0)), wc3::Real(wc3::GetUnitFlyHeight(unit_)),
            yaw_,wc3::Real(wc3::GetCameraField(5)),mouseLook_.Raw());
    }
}
