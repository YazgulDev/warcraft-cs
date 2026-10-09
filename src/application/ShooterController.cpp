#include "ShooterController.hpp"
#include "../platform/DiagnosticLog.hpp"
#include "../combat/CombatDamage.hpp"
#include "../platform/MapCameraGuard.hpp"
#include "../platform/SpriteTransform.hpp"
#include "../input/LookAngles.hpp"
#include "../platform/FpsCombatGuard.hpp"
#include "../economy/BuyAccess.hpp"
#include "../presentation/BuyMenuLayout.hpp"
#include "../platform/MapEnvironment.hpp"
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
    collision_.Configure(gameBase);
    audio_.Configure(root_);
    if (!nativeSky_.Configure(gameBase)) wc3::Log("Native sky signature mismatch; preview disabled");
    ReloadSettings();
    ResetLoadout();
}
void ShooterController::ResetLoadout() {
    // One kit per map/runtime, not per F6: mode switching cannot create free guns or charges.
    int magazines[WeaponSlots::Count],reserves[WeaponSlots::Count];
    for (int i=0;i<WeaponSlots::Count;++i) { magazines[i]=weapons[i].magazine;reserves[i]=weapons[i].reserve; }
    inventory_.Reset(settings_,ammo_,reserve_,magazines,reserves);
    // The default kit includes a loaded USP, selected when a fresh map starts.
    weapon_=settings_.startAllWeapons ? 0 : WeaponSlots::Pistol;
}
void ShooterController::Notice(const char* message) { ammoMessage_=message;refillTick_=GetTickCount(); }
void ShooterController::RequestBuyClick(int x,int y,int width,int height) {
    if (Buying()) buyKeyRequested_=BuyMenuLayout::Key(x,y,width,height,buyMenu_.Current());
}
void ShooterController::Buy(int slot,bool ammunition) {
    if (status_.incapacitated || status_.contained || status_.hidden) { Notice("CANNOT BUY WHILE DISABLED");return; }
    if (!BuyAccess::Allowed(unit_,settings_)) { Notice("OUTSIDE BUY ZONE");return; }
    if (slot<0 || slot>=WeaponSlots::Count) return;
    auto owner=wc3::GetOwningPlayer(unit_);int gold=wc3::GetPlayerState(owner,1);
    auto result=inventory_.Buy(slot,ammunition,settings_,gold,ammo_[slot],reserve_[slot],weapons[slot].magazine,weapons[slot].reserve);
    // Native gold is charged only after validation, once on the game's main thread.
    if (result==BuyInventory::Result::Bought) {
        wc3::SetPlayerState(owner,1,gold);gold_=gold;Notice(ammunition ? "AMMUNITION PURCHASED" : "WEAPON PURCHASED");
        wc3::Log("buy weapon=%s ammunition=%d gold=%d ammo=%d reserve=%d",weapons[slot].name,ammunition,gold,ammo_[slot],reserve_[slot]);
        if (!ammunition) SwitchWeapon(slot);
        // Successful CS purchases return directly to play; rejected purchases leave the menu open.
        buyMenu_.Close();
    } else Notice(result==BuyInventory::Result::NoGold ? "NOT ENOUGH GOLD" : result==BuyInventory::Result::Full ? "AMMUNITION FULL" :
        result==BuyInventory::Result::AlreadyOwned ? "ALREADY OWNED" : "NO AMMUNITION FOR THIS WEAPON");
}
void ShooterController::UpdateBuyMenu() {
    bool wasBuying=Buying();
    gold_=wc3::GetPlayerState(wc3::GetOwningPlayer(unit_),1);
    if (status_.contained || status_.hidden) { buyMenu_.Close();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;return; }
    if (buyToggleRequested_) {
        if (Buying() || BuyAccess::Allowed(unit_,settings_)) {
            buyMenu_.Toggle();mouseLook_.Reset();weaponWheel_.Reset();suppressFire_=true;CancelPlant();meleeContact_=0;
            reloadEnd_=0;audio_.CancelAnimation();Play("idle");
        } else Notice("OUTSIDE BUY ZONE");
    }
    if (buyAmmoRequested_) Buy(weapon_,true);
    if (Buying() && !BuyAccess::Allowed(unit_,settings_)) { buyMenu_.Close();Notice("LEFT BUY ZONE"); }
    if (Buying() && buyKeyRequested_>=0) {
        auto action=buyMenu_.Select(buyKeyRequested_,weapon_);
        // Original CS splits primary (rifles) and secondary (pistols) ammunition.
        if (action.primaryAmmo || action.secondaryAmmo) {
            auto category=action.primaryAmmo ? BuyCatalog::Category::Rifles : BuyCatalog::Category::Pistols;
            bool carried=false;
            for (const auto& item : BuyCatalog::Entries) if (item.category==category && inventory_.owned[item.slot]) {
                Buy(item.slot,true);carried=true;
            }
            if (!carried) Notice("NO WEAPON IN THIS CATEGORY");
        }
        if (action.slot>=0) Buy(action.slot,false);
    }
    // Closing via a clicked 0 row must not turn that same held button into a gunshot/C4 installation.
    if (wasBuying && !Buying()) { mouseLook_.Reset();suppressFire_=true; }
    buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;
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
void ShooterController::InterfaceRequest(bool show) {
    cinematicRequested_ = !show;
    if (active_ && !show) {
        nativeSky_.Update(ui_,false,mapTileset_);
        // Yield before the map applies its cinematic layout/camera; keep the mission unit visible.
        audio_.Stop(); movement_.Stop(); suspended_ = true;
        wc3::Log("FPS yielded to map cinematic");
    }
}
void ShooterController::Disable(bool restoreCamera) {
    buyMenu_.Close();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;
    mouseLook_.Reset();
    squad_.Release(); // Native RTS regains followers and their ordinary attack policy on F6/F10.
    if (restoreCamera) {
        // Restoration is our own camera write even though FPS is still active until cleanup completes.
        ownCameraRequest_ = true;
        float time = 0.25f; wc3::ResetToGameCamera(&time);
        ownCameraRequest_ = false;
    }
    CancelPlant();
    active_ = false; scopeLevel_ = 0; reloadEnd_ = 0;
    nativeSky_.Update(ui_,false,mapTileset_);
    meleeContact_ = meleeReady_ = 0; recoil_.Reset();
    audio_.Stop(); stepDistance_ = 0;
    hits_.Clear(); refillRequested_ = false; allWeaponsRequested_=false; refillTick_ = 0;
    itemRequested_=settingsRequested_=itemNearby_=false;squadRequested_=0;
    movement_.Stop();
    weaponWheel_.Reset(); // A partial notch must not survive leaving FPS.
    status_ = {};
    wc3::Log("FPS disabled");
}
void ShooterController::ResetMap() {
    collision_.Reset(); // Custom-map model bounds must never leak into the next world's collision.
    nativeSky_.Reset();
    mapTileset_=0;
    mouseLook_.Reset();
    weaponWheel_.Reset();
    squad_.Reset(); // Forget unloaded handles without issuing commands into the next map.
    // Never dereference a unit handle after a map unload: campaign transitions reuse IDs.
    hitboxes_.Reset(); destructableHitboxes_.Reset(); bombs_.Reset(); plantProgress_ = 0;
    buyMenu_.Close();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;
    fixtureGate_ = 0;
    unit_ = 0; active_ = false; suspended_ = false; paused_ = false; ui_ = 0; tick_ = 0;
    scopeLevel_ = 0; cinematicRequested_ = false; toggleRequested_ = false; ownCameraRequest_ = false;
    status_ = {}; meleeContact_ = meleeReady_ = 0; recoil_.Reset();
    std::fill(std::begin(keys_), std::end(keys_), false); lastShot_ = 0;
    audio_.Stop(); stepDistance_ = 0; reloadEnd_ = 0;
    menuRequested_ = false;
    itemRequested_=settingsRequested_=itemNearby_=false;squadRequested_=0;ammoRecovery_.Reset();
    hits_.Clear(); refillRequested_ = false; allWeaponsRequested_=false; refillTick_ = 0; fixtureTarget_ = 0; blastFixtures_.clear();
    ResetLoadout();
}
void ShooterController::Toggle() {
    if (active_) { Disable(true); return; }
    if (!wc3::SinglePlayer()) { wc3::Log("FPS refused: requires one human player"); return; }
    unit_ = wc3::PickOwnedUnit();
    if (!unit_) { wc3::Log("FPS needs a living owned unit or hero"); return; }
    // Storm searches the current map archive; cache its tileset only after a live map/unit is available.
    mapTileset_=MapEnvironment::Tileset();
    wc3::Log("FPS map tileset=%c sky=%s",mapTileset_ ? mapTileset_ : '?',SkyName().c_str());
    yaw_ = wc3::Real(wc3::GetUnitFacing(unit_)); pitch_ = 0;
    // F6 begins a fresh capture rather than applying the RTS cursor's distance from the center.
    mouseLook_.Reset();
    // Begin on the native standing surface, including walkable bridges, independent of the RTS speed cap.
    weaponWheel_.Reset();
    movement_.Reset(wc3::Ground(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_))));
    wc3::IssueImmediateOrderById(unit_, 851972);  // stop autonomous attack/movement orders
    float zero = 0; wc3::SetCameraTargetController(0, &zero, &zero, FALSE);
    // Keep native UI/portrait anchors intact; FPS suppresses HUD graphics at GL submission.
    active_ = true; suspended_ = false; scopeLevel_ = 0; Play("draw");
    wc3::Log("FPS enabled: unit=%08X type=%08X", unit_, wc3::GetUnitTypeId(unit_));
}
void ShooterController::AimAndMove(float dt) {
    // A contained unit belongs to native cargo: changing XY/stance would fake an escape without releasing it.
    if (status_.contained || status_.hidden) { mouseLook_.Reset();movement_.Stop(); stepDistance_ = 0; return; }
    // Mouse capture occurs only in the foreground and only after the player enables FPS.
    float dx=0,dy=0;
    if (Buying()) mouseLook_.Reset();else mouseLook_.Sample(dx,dy);
    if (!status_.incapacitated && !Buying()) {
        float sensitivity = scopeLevel_ == 2 ? 0.009f : Scoped() ? 0.035f : 0.14f;
        LookAngles::Apply(yaw_,pitch_,dx,dy,sensitivity);
    }
    MoveInput input;
    input.forward = float(Down('W')) - float(Down('S'));
    input.right = float(Down('D')) - float(Down('A')); input.yaw = yaw_;
    input.walk = Down(VK_SHIFT); input.duck = Down(VK_CONTROL); input.jump = Down(VK_SPACE);
    // Menu navigation stops horizontal input/look without pausing the Warcraft world or gravity.
    if (Buying()) { input.forward=input.right=0;input.jump=false;input.duck=movement_.Duck()>=.5f; }
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
        // Transition records expose retained takeoff/landing momentum without per-frame disk writes.
        if (tookOff || ((!wasGrounded || tookOff) && movement_.Grounded()))
            wc3::Trace("movement contact takeoff=%d landed=%d speed=%.1f goldSrcSpeed=%.1f velocity=%.1f,%.1f,%.1f forward=%.0f right=%.0f yaw=%.2f",
                tookOff, movement_.Grounded(), movement_.Speed(), movement_.Speed()/MovementPhysics::worldScale,
                movement_.VelocityX(), movement_.VelocityY(), movement_.VerticalVelocity(), input.forward, input.right, yaw_);
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
            reloadEnd_ = 0; meleeContact_ = 0; refillRequested_ = false; allWeaponsRequested_=false; CancelPlant(); audio_.Stop(); Play("idle");
        }
    }
    if ((status_.contained || status_.hidden) && !current.contained && !current.hidden) {
        // On unloading/rescue, resume from the actual native position, not the stale pre-swallow eye height.
        movement_.Reset(wc3::Ground(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_))));
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
    // Body/attachments are hidden by the world-only render filter; preserve native portrait tint/alpha.
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
void ShooterController::RefillAmmo(bool grantAll) {
    // F7 freely restores carried ammo; F9 additionally unlocks every slot without spending gold.
    int magazines[WeaponSlots::Count],reserves[WeaponSlots::Count];
    for (int i=0;i<WeaponSlots::Count;++i) { magazines[i]=weapons[i].magazine;reserves[i]=weapons[i].reserve; }
    inventory_.Refill(settings_,ammo_,reserve_,magazines,reserves,grantAll);
    if (reloadEnd_) { reloadEnd_ = 0; audio_.CancelAnimation(); Play("idle"); }
    CancelPlant();ammoRecovery_.Reset();
    Notice(grantAll ? "ALL WEAPONS AND AMMO GRANTED" : "ALL AMMO RESTORED");
    wc3::Log("%s free inventory: AK47=%d/%d M4A1=%d/%d USP=%d/%d AWP=%d/%d C4=%d gold=%d",
        grantAll ? "F9" : "F7",ammo_[0],reserve_[0],ammo_[1],reserve_[1],ammo_[2],reserve_[2],ammo_[3],reserve_[3],ammo_[5],gold_);
}
void ShooterController::SwitchWeapon(int slot) {
    if (slot<0 || slot>=WeaponSlots::Count || !inventory_.owned[slot] || weapon_ == slot) return;
    // Number keys and the wheel share deployment: cancel reload, scope, planting and pending melee cues.
    CancelPlant(); audio_.CancelAnimation(); meleeContact_ = meleeReady_ = 0;
    recoil_.ResetBurst(); weapon_ = slot; reloadEnd_ = 0; scopeLevel_ = 0; Play("draw");
    wc3::Log("weapon selected=%s slot=%d", weapons[slot].name, slot + 1);
}
void ShooterController::ReloadSettings() {
    // Replace one complete snapshot; stale fractional credit must not survive a percentage/scope change.
    settings_=GameplaySettings::Load(root_+"\\WarcraftCS.ini");
    // Startup and F8 share validated logging settings without discarding the active session.
    DiagnosticLog::Configure(settings_.logging);
    wc3::Log("Logging configured detailed=%d intervalMs=%u maxFileMB=%u archives=%u",
        settings_.logging.detailed, settings_.logging.intervalMs, settings_.logging.maxFileMB, settings_.logging.archiveCount);
    // Startup and F8 share the same CS-only gain update; Warcraft's Miles mixer is untouched.
    audio_.SetVolumePercent(settings_.csVolumePercent);
    movement_.Configure(settings_.movement); // F8 changes tuning without resetting a jump or accumulated momentum.
    wc3::Trace("settings movement bunnyHop=%d autoJump=%d jumpBoostPercent=%.1f maxBunnySpeed=%.1f airAcceleration=%.1f jumpSpeed=%.3f gravity=%.1f stepHeight=%.1f",
        settings_.movement.bunnyHop, settings_.movement.autoJump, settings_.movement.jumpBoostPercent,
        settings_.movement.maxBunnySpeed, settings_.movement.airAcceleration, settings_.movement.jumpSpeed,
        settings_.movement.gravity, settings_.movement.stepHeight);
    ammoRecovery_.Reset();ammoMessage_="SETTINGS RELOADED";refillTick_=GetTickCount();
    wc3::Log("Settings loaded runePercent=%.1f ammoWeapons=%s damageMode=%s awpOneShot=%d radius=%.1f friendlyFirePercent=%.1f",
        settings_.runeAmmoPercent,settings_.runeAmmoAllWeapons ? "all" : "current",
        settings_.heroDamage ? "hero" : "weapon",settings_.awpOneShot,settings_.runePickupRadius,settings_.friendlyFirePercent);
    // Log the validated snapshot, including prices/sky choices, rather than arbitrary raw INI contents.
    wc3::Trace("settings loadoutAll=%d bombs=%d/%d buyAccess=%d buyRadius=%.1f shops=%s csVolume=%.1f floatingText=%.1f squadRadius=%.1f follow=%.1f leash=%.1f maxUnits=%d csSky=%d nativeSky=%d defaultSky=%s",
        settings_.startAllWeapons, settings_.startBombs, settings_.maxBombs, int(settings_.buyAccess), settings_.buyRadius,
        settings_.shopTypes.c_str(), settings_.csVolumePercent, settings_.floatingTextDistance, settings_.squadRadius,
        settings_.squadFollowDistance, settings_.squadLeash, settings_.squadMaxUnits, settings_.csSky, settings_.nativeSky, settings_.defaultSky.c_str());
    for (int slot = 0; slot < WeaponSlots::Count; ++slot)
        wc3::Trace("settings weapon=%s damage=%.1f heroMultiplier=%.2f price=%d ammoPrice=%d ammoPack=%d",
            weapons[slot].name, settings_.damage[slot], settings_.heroMultiplier[slot], settings_.weaponPrice[slot], settings_.ammoPrice[slot], settings_.ammoPack[slot]);
    wc3::Trace("settings secondaryDamage knife=%.1f sword=%.1f", settings_.knifeSecondaryDamage, settings_.swordSecondaryDamage);
    for (unsigned code = 0; code < settings_.tilesetSky.size(); ++code)
        if (!settings_.tilesetSky[code].empty()) wc3::Trace("settings tileset=%c sky=%s", code, settings_.tilesetSky[code].c_str());
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
        if (!inventory_.owned[slot]) continue;
        if (!settings_.runeAmmoAllWeapons && slot!=weapon_) continue;
        added+=ammoRecovery_.Restore(slot,settings_.runeAmmoPercent,slot==WeaponSlots::C4 ? settings_.maxBombs : weapons[slot].magazine,weapons[slot].reserve,ammo_[slot],reserve_[slot]);
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
        // Alliance changes are evaluated at contact; friendly AWP scales ordinary damage instead of finishing.
        // Hero mode applies the current attack and per-weapon multiplier at contact; friendly reduction follows it.
        float attack=settings_.heroDamage ? wc3::UnitAttackAverage(unit_) : 0;
        float baseDamage=settings_.ContactDamage(weapon_,WeaponSlots::Melee(weapon_) && meleeAttack_.secondary,attack);
        float damage = CombatDamage::Amount(baseDamage,allied,lethal,before,settings_.friendlyFirePercent);
        if (settings_.heroDamage) wc3::Log("hero damage attack=%.1f multiplier=%.2f base=%.1f",attack,settings_.heroMultiplier[weapon_],baseDamage);
        // Universal AWP damage ignores Warcraft armor; native death keeps death triggers intact.
        // Disabling friendly fire also avoids zero-damage map events and reflected/retaliatory side effects.
        if (damage>0) wc3::UnitDamageTarget(unit_, target, &damage, TRUE, TRUE, 0, lethal ? 26 : 4, 0);
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
        ammo_[WeaponSlots::C4] <= 0) { CancelPlant(); return; }
    if (plantProgress_ == 0) { plantX_ = x; plantY_ = y; Play("pressbutton"); }
    if (std::abs(x - plantX_) + std::abs(y - plantY_) > 4) { CancelPlant(); return; }
    plantProgress_ += dt;
    if (plantProgress_ >= 3) {
        if (bombs_.Plant(unit_, x, y, audio_,settings_.damage[WeaponSlots::C4],settings_.friendlyFirePercent)) { --ammo_[WeaponSlots::C4]; Play("drop"); }
        plantProgress_ = 0;
    }
}
const char* ShooterController::Animation() const { return animation_.c_str(); }
float ShooterController::AnimationTime() const { return (GetTickCount() - animationTick_) * 0.001f * animationRate_; }
void ShooterController::Tick(uintptr_t ui) {
    if (ui_ && ui != ui_) ResetMap(); ui_ = ui;
    DWORD now = GetTickCount(); float dt = tick_ ? std::min((now - tick_) * 0.001f, 0.05f) : 0; tick_ = now;
    DWORD foregroundPid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
    if (bombs_.Tick(audio_) && Visible()) hits_.Record();
    if (foregroundPid != GetCurrentProcessId()) {
        // Drop background packets; refocusing establishes a fresh cursor anchor without a view jump.
        mouseLook_.Reset();
        weaponWheel_.Reset();
        itemRequested_=false;squadRequested_=0; // Commands pressed before losing focus must not execute on return.
        buyMenu_.Close();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;
        CancelPlant(); meleeContact_ = 0;
        // Cancel queued animation events rather than releasing delayed reload sounds on return.
        audio_.CancelAnimation(); return;
    }
    // Map scripts still control cinematic input/layout; FPS leaves native HUD state intact.
    // Map-script interface requests are distinct from the FPS mode's own hidden HUD.
    bool cinematic = cinematicRequested_;
    // Native menus have their own pause flags, separate from the JASS PauseGame callback.
    bool suspended = cinematic || paused_ || *reinterpret_cast<int*>(ui + 0x258) || *reinterpret_cast<int*>(ui + 0x260);
    if (active_ && suspended != suspended_) {
        // Yield actor visibility and layout to native menus/cinematics; mission bounds stay untouched.
        audio_.Stop(); stepDistance_ = 0;
        wc3::Log("FPS %s for campaign/UI", suspended ? "suspended" : "resumed");
    }
    suspended_ = suspended;
    if (suspended_) { CancelPlant(); meleeContact_ = 0;buyMenu_.Close();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1; }
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
    // Menus/cinematics return ownership to the map; live FPS may temporarily fill a missing native sky.
    nativeSky_.Update(ui_,Visible() && settings_.nativeSky && !settings_.csSky,mapTileset_);
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
    bool escape=Pressed(VK_ESCAPE);
    if (escape && Buying()) { buyMenu_.Close();mouseLook_.Reset();suppressFire_=true;escape=false; }
    if (escape || !wc3::GetUnitTypeId(unit_) || wc3::Real(wc3::GetUnitState(unit_, 0)) <= 0.405f) { Disable(true); return; }
    health_ = wc3::Real(wc3::GetUnitState(unit_, 0));
    UpdateStatus();
    // J gives followers back to native control even when their FPS leader cannot act.
    if (squadRequested_==3) {
        squad_.Release();squadRequested_=0;ammoMessage_="SQUAD RELEASED";refillTick_=now;
        wc3::Log("squad released by J: count=%u",unsigned(squad_.Count()));
    }
    recoil_.Step(dt, Down(VK_LBUTTON) && !status_.incapacitated && !status_.disarmed, weapon_);
    if (status_.incapacitated) {
        buyMenu_.Close();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;
        // Discard scrolling during disabled states rather than replaying a switch after recovery.
        weaponWheel_.Reset();
        // Disabled leaders cannot queue pickup/recruitment for later; existing followers retain their chosen policy.
        itemRequested_=false;squadRequested_=0;
        squad_.Tick(unit_,settings_,now,!status_.contained && !status_.hidden);
        // Consume action edges while disabled, so held reload/scope/switch inputs are not replayed on recovery.
        static const int actionKeys[] = {'1', '2', '3', '4', '5', '6', '7', 'R', VK_LBUTTON, VK_RBUTTON};
        for (int key : actionKeys) Pressed(key);
        refillRequested_ = false; allWeaponsRequested_=false;
        AimAndMove(dt); Camera(); audio_.Tick(GetTickCount());
        return;
    }
    if (allWeaponsRequested_) { allWeaponsRequested_=false;refillRequested_=false;RefillAmmo(true); }
    if (refillRequested_) { refillRequested_ = false; RefillAmmo(); }
    UpdateBuyMenu();
    if (!Down(VK_LBUTTON) && !Down(VK_RBUTTON)) suppressFire_=false;
    if (Buying()) {
        itemRequested_=false;squadRequested_=0; // Menu taps cannot queue RTS interaction for after closing.
        for (int key='0';key<='9';++key) Pressed(key);
        Pressed(VK_LBUTTON);Pressed(VK_RBUTTON);Pressed('R');
        if (!status_.contained && !status_.hidden) FpsCombatGuard::CancelOrders(unit_);
        squad_.Tick(unit_,settings_,now,!status_.contained && !status_.hidden);
        movement_.Stop();weaponWheel_.Reset();AimAndMove(dt);Camera();audio_.Tick(now);return;
    }
    // Wheel packets are consumed once; an explicit number key takes precedence in the same frame.
    int wheel=weaponWheel_.Take(weapon_);
    int steps=(wheel-weapon_+WeaponSlots::Count)%WeaponSlots::Count;
    if (steps>WeaponSlots::Count/2) steps-=WeaponSlots::Count;
    SwitchWeapon(inventory_.Next(weapon_,steps));
    for (int i = 0; i < WeaponSlots::Count; ++i) if (Pressed('1' + i)) SwitchWeapon(i);
    if (Pressed('R')) Reload();
    // Consume the right-click edge for every weapon so holding it across a switch never scopes or stabs.
    bool secondaryPressed = Pressed(VK_RBUTTON);
    if (!suppressFire_ && weapon_ == 3 && secondaryPressed) {
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
    if (!suppressFire_ && weapon_ == WeaponSlots::C4) PlantC4(dt);
    else if (!suppressFire_ && !status_.disarmed && WeaponSlots::Melee(weapon_) && secondaryPressed) Fire(true);
    else if (!suppressFire_ && !status_.disarmed && ((weapons[weapon_].automatic && Down(VK_LBUTTON)) || firePressed)) Fire();
    // Sample after any newly started animation: an older tick would wrap unsigned elapsed time.
    Camera(); audio_.Tick(GetTickCount());
    // A configurable summary observes physics/camera without writing every simulation frame.
    static DWORD lastState = 0;
    if (DiagnosticLog::Due(lastState)) {
        wc3::Trace("movement x=%.1f y=%.1f ground=%.1f feet=%.1f eye=%.1f speed=%.1f duck=%.2f grounded=%d cameraEye=%.1f cameraX=%.1f cameraY=%.1f pitch=%.1f distance=%.1f fly=%.1f yaw=%.3f cameraYaw=%.3f rawMouse=%d",
            wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_)),
            wc3::Ground(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_))),
            movement_.FeetZ(), movement_.EyeZ(), movement_.Speed(), movement_.Duck(), movement_.Grounded(),
            wc3::Real(wc3::GetCameraEyePositionZ()), wc3::Real(wc3::GetCameraEyePositionX()),
            wc3::Real(wc3::GetCameraEyePositionY()), pitch_, wc3::Real(wc3::GetCameraField(0)), wc3::Real(wc3::GetUnitFlyHeight(unit_)),
            yaw_,wc3::Real(wc3::GetCameraField(5)),mouseLook_.Raw());
    }
}
