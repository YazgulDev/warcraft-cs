#include "ShooterController.hpp"
#include "../platform/DiagnosticLog.hpp"
#include "../combat/CombatDamage.hpp"
#include "../combat/UnitScaling.hpp"
#include "../platform/MapCameraGuard.hpp"
#include "../platform/SpriteTransform.hpp"
#include "../input/LookAngles.hpp"
#include "../platform/FpsCombatGuard.hpp"
#include "../platform/NativeWeaponAttack.hpp"
#include "../platform/WeaponTargetFilter.hpp"
#include "../economy/BuyAccess.hpp"
#include "../presentation/BuyMenuLayout.hpp"
#include "../presentation/CursorAimRay.hpp"
#include "../platform/MapEnvironment.hpp"
#ifdef WCS_GAMEPLAY_TEST
#include "../../tests/BackgroundGameplay.hpp"
#endif
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
    spells_.Configure(gameBase);itemInventory_.Configure(gameBase);
    unitHudReader_.Configure(gameBase); // Verify map-resolved XP and item art before sampling the actor.
    // Native-only clients do not need a CS mixer or any converted cache to initialize.
    settings_=GameplaySettings::Load(root_+"\\WarcraftCS.ini");
    if(!settings_.warcraftOnly)audio_.Configure(root_);
    if (!nativeSky_.Configure(gameBase)) wc3::Log("Native sky signature mismatch; preview disabled");
    ReloadSettings();
    ResetLoadout();
}
void ShooterController::ResetLoadout() {
    // One kit per map/runtime, not per F6: mode switching cannot create free guns or charges.
    int magazines[WeaponSlots::Count],reserves[WeaponSlots::Count];
    for (int i=0;i<WeaponSlots::Count;++i) { magazines[i]=weapons[i].magazine;reserves[i]=settings_.maxReserve[i]; }
    inventory_.Reset(settings_,ammo_,reserve_,magazines,reserves);
    // The default kit includes a loaded USP, selected when a fresh map starts.
    weapon_=settings_.startAllWeapons ? 0 : WeaponSlots::Pistol;
}
void ShooterController::Notice(const char* message) { ammoMessage_=message;refillTick_=GetTickCount(); }
void ShooterController::RequestBuyClick(int x,int y,int width,int height) {
    if (Buying()) buyKeyRequested_=BuyMenuLayout::Key(x,y,width,height,buyMenu_.Current());
}
void ShooterController::Buy(int slot,bool ammunition) {
    if(settings_.warcraftOnly)return; // All purchase entry points honor the installed mode set.
    if (status_.incapacitated || status_.contained || status_.hidden) { Notice("CANNOT BUY WHILE DISABLED");return; }
    if (!BuyAccess::Allowed(unit_,settings_)) { Notice("OUTSIDE BUY ZONE");return; }
    if (slot<0 || slot>=WeaponSlots::Count) return;
    auto owner=wc3::GetOwningPlayer(unit_);int gold=wc3::GetPlayerState(owner,1);
    auto result=inventory_.Buy(slot,ammunition,settings_,gold,ammo_[slot],reserve_[slot],weapons[slot].magazine,settings_.maxReserve[slot]);
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
    if (status_.contained || status_.hidden) { buyMenu_.Close();itemInventory_.ClearInput();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;return; }
    if (buyToggleRequested_) {
        itemInventory_.ClearInput(); // The weapon catalog and mouse inventory have exclusive cursor ownership.
        if (Buying() || BuyAccess::Allowed(unit_,settings_)) {
            buyMenu_.Toggle();mouseLook_.Reset();weaponWheel_.Reset();cameraWheel_.Reset();suppressFire_=true;CancelPlant();meleeContact_=0;
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
bool ShooterController::Down(int key) const {
#ifdef WCS_GAMEPLAY_TEST
    // Background fixtures must never interpret keys typed into the user's other applications.
    if(BackgroundGameplay::Enabled())return (key==VK_LBUTTON && GetTickCount()<testAttackUntil_) ||
        ((key==testWalkKey_ || (testWalking_ && key==settings_.shortcuts.Key(ShortcutAction::Walk))) && GetTickCount()<testWalkUntil_);
#endif
    return key>0 && key<256 && (GetAsyncKeyState(key) & 0x8000) != 0;
}
bool ShooterController::Pressed(int key) { if(key<=0 || key>=256)return false;bool down = Down(key), rising = down && !keys_[key]; keys_[key] = down; return rising; }
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
    if(settings_.warcraftOnly)return 0;
    animation_ = Sequence(name); animationTick_ = GetTickCount();
    // The supplied sword keeps its authored frame rate; only the retail knife is fitted to its cadence.
    animationRate_ = WeaponSlots::Melee(weapon_) && animation_.find("midslash") == 0 ? (weapon_ == WeaponSlots::Sword ? 1.0f : 2.5f) : 1.0f;
    return audio_.BeginAnimation(weapon_, animation_.c_str(), animationTick_);
}
void ShooterController::Sound(const char* name) {
    if(settings_.warcraftOnly)return;
    audio_.Play(name);
}
void ShooterController::SurfaceStep(float volume) {
    if(UnitFirstPerson() || settings_.warcraftOnly)return; // Native-only views retain Warcraft's own audio.
    // Use CS's default concrete samples for steps, jump push-off and normal ground contact.
    static const char* steps[] = {"pl_step1.wav", "pl_step3.wav", "pl_step2.wav", "pl_step4.wav"};
    audio_.Play(steps[stepIndex_++ % 4], volume);
}
void ShooterController::InterfaceRequest(bool show) {
    cinematicRequested_ = !show;
    if (active_ && !show) {
        unitLocomotion_.Release(); // Native cinematics own animation playback immediately.
        nativeSky_.Update(ui_,false,mapTileset_);
        // Yield before the map applies its cinematic layout/camera; keep the mission unit visible.
        CancelCasting();spellRequested_=-1;spellCancelRequested_=false;
        itemInventory_.ClearInput();audio_.Stop(); movement_.Stop(); suspended_ = true;
        wc3::Log("FPS yielded to map cinematic");
    }
}
void ShooterController::Disable(bool restoreCamera) {
    unitLocomotion_.Release();
    heroAttack_.Cancel();viewToggleRequested_=thirdPersonRequested_=unitViewRequested_=false;
    if(ThirdPerson() && unit_ && wc3::GetUnitTypeId(unit_))wc3::Animate(unit_,"stand");
    view_=ControlView::WeaponsFirstPerson;
    CancelCasting();spellRequested_=-1;spellCancelRequested_=false;
    healthBarToggleRequested_=false; // Do not replay a pending bar switch on the next F6.
    allyTargetToggleRequested_=false;
    // Restore the current map before native RTS/menu input takes over.
    nativeFog_.Update(false);fogToggleRequested_=false;
    buyMenu_.Close();itemInventory_.ClearInput();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;
    mouseLook_.Reset();
    squadOrderMode_.ClearInput();squad_.Release(); // Native RTS regains followers and their ordinary attack policy on F6/F10.
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
    // Discard pending display taps without losing the session's chosen visibility.
    itemRequested_=settingsRequested_=itemNearby_=speedToggleRequested_=minimapToggleRequested_=false;squadRequested_=0;
    movement_.Stop();
    weaponWheel_.Reset();cameraWheel_.Reset(); // A partial notch must not survive leaving FPS.
    status_ = {};
    wc3::Log("FPS disabled");
}
void ShooterController::ResetMap() {
    thirdPersonDistance_=settings_.thirdPerson.distance; // Each new map begins with its configured zoom.
    unitLocomotion_.Reset(); // Map handles are already unloaded: discard, do not restore stale actors.
    heroAttack_.Reset();view_=ControlView::WeaponsFirstPerson;viewToggleRequested_=thirdPersonRequested_=unitViewRequested_=false;
    spellHud_={};spellHudTick_=spellHudLog_=0; // Forget per-map art and cooldown snapshots with their ability handles.
    unitHud_={};unitHudTick_=0; // Never display an unloaded map's portrait stats or items.
    spells_.Reset();itemInventory_.Reset();spellRequested_=-1;spellCancelRequested_=false; // Forget unloaded-map orders without touching stale handles.
    healthBarToggleRequested_=false;
    allyTargetToggleRequested_=false;
    nativeFog_.Reset();fogToggleRequested_=false; // An unloaded map's fog snapshot must be forgotten.
    collision_.Reset(); // Custom-map model bounds must never leak into the next world's collision.
    nativeSky_.Reset();
    mapTileset_=0;
    mouseLook_.Reset();
    weaponWheel_.Reset();cameraWheel_.Reset();
    squad_.Reset();squadOrderMode_.Configure(settings_.squadReticleAttackInFPS); // New maps restore the configured initial F6 mode.
    // Never dereference a unit handle after a map unload: campaign transitions reuse IDs.
    hitboxes_.Reset(); destructableHitboxes_.Reset(); bombs_.Reset(); plantProgress_ = 0;
    buyMenu_.Close();itemInventory_.ClearInput();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;
    fixtureGate_ = 0;
    unit_ = 0; active_ = false; suspended_ = false; paused_ = false; ui_ = 0; tick_ = {};
    scopeLevel_ = 0; cinematicRequested_ = false; toggleRequested_ = false; ownCameraRequest_ = false;
    status_ = {}; meleeContact_ = meleeReady_ = 0; recoil_.Reset();
    std::fill(std::begin(keys_), std::end(keys_), false); lastShot_ = 0;
    audio_.Stop(); stepDistance_ = 0; reloadEnd_ = 0;
    menuRequested_ = false;
    itemRequested_=settingsRequested_=itemNearby_=speedToggleRequested_=minimapToggleRequested_=false;squadRequested_=0;ammoRecovery_.Reset();
    hits_.Clear(); refillRequested_ = false; allWeaponsRequested_=false; refillTick_ = 0; fixtureTarget_ = 0; blastFixtures_.clear();
    ResetLoadout();
}
void ShooterController::Toggle(ControlView initial) {
    if (active_) { Disable(true); return; }
    if(settings_.warcraftOnly && initial==ControlView::WeaponsFirstPerson) {
        wc3::Log("Weapon FPS disabled by installation profile; use F7 or F9");return;
    }
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
    weaponWheel_.Reset();cameraWheel_.Reset();
    movement_.Reset(wc3::Ground(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_))));
    wc3::IssueImmediateOrderById(unit_, 851972);  // stop autonomous attack/movement orders
    float zero = 0; wc3::SetCameraTargetController(0, &zero, &zero, FALSE);
    // Keep native UI/portrait anchors intact; FPS suppresses HUD graphics at GL submission.
    view_=initial;
    if(ThirdPerson())pitch_=settings_.thirdPerson.initialPitch;
    active_ = true; suspended_ = false; scopeLevel_ = 0;
    if(initial==ControlView::WeaponsFirstPerson)Play("draw");
    wc3::Log("FPS enabled: unit=%08X type=%08X", unit_, wc3::GetUnitTypeId(unit_));
}
void ShooterController::ToggleView() {
    // T cycles only installed views; the reduced client can never enter an empty weapon view.
    SelectView(ThirdPerson() ? (settings_.warcraftOnly ? ControlView::UnitFirstPerson : ControlView::WeaponsFirstPerson) : ControlView::ThirdPerson);
}
void ShooterController::SelectView(ControlView next) {
    if(!Visible())return;
    if(next==ControlView::WeaponsFirstPerson && settings_.warcraftOnly)return;
    if(next==ControlView::UnitFirstPerson && !settings_.unitViewEnabled)return;
    if(next==ControlView::ThirdPerson && !settings_.thirdPerson.enabled) {
        Notice("THIRD PERSON DISABLED IN CONFIG");return;
    }
    // Inventory and physical position stay intact; no queued gunshot or delayed swing crosses the transition.
    unitLocomotion_.Release();unitLocomotion_.Reset();
    CancelCasting();heroAttack_.Cancel();CancelPlant();meleeContact_=0;reloadEnd_=0;scopeLevel_=0;
    buyMenu_.Close();itemInventory_.ClearInput();recoil_.Reset();audio_.Stop();mouseLook_.Reset();weaponWheel_.Reset();cameraWheel_.Reset();suppressFire_=true;
    // Pending combat/UI mailboxes cannot execute after entering the unarmed view.
    refillRequested_=allWeaponsRequested_=itemRequested_=buyToggleRequested_=buyAmmoRequested_=false;
    buyKeyRequested_=spellRequested_=-1;spellCancelRequested_=false;
    view_=next;pitch_=ThirdPerson() ? settings_.thirdPerson.initialPitch : 0;
    squadRequested_=0;squadOrderMode_.ClearInput();if(!SquadReticleOrdersEnabled())squad_.CancelFocus(); // No queued click crosses a view change.
    if(!ThirdPerson()) {wc3::Animate(unit_,"stand");if(!UnitFirstPerson())Play("draw");}
    wc3::Log("View mode=%s unit=%08X",UnitFirstPerson() ? "unit-first-person" : ThirdPerson() ? "third-person" : "first-person",unit_);
    Notice(UnitFirstPerson() ? "FIRST PERSON: UNIT VIEW" : ThirdPerson() ? "THIRD PERSON: UNIT CONTROL" : "FIRST PERSON: CS WEAPONS");
}
void ShooterController::AimRay(float* origin,float* direction) {
    float yaw=ViewYaw()*radians,pitch=ViewPitch()*radians;
    origin[0]=wc3::Real(wc3::GetUnitX(unit_));origin[1]=wc3::Real(wc3::GetUnitY(unit_));
    origin[2]=movement_.EyeZ()+wc3::Real(wc3::GetUnitFlyHeight(unit_));
    for(int i=0;i<3;++i)direction[i]=i==0 ? std::cos(pitch)*std::cos(yaw) : i==1 ? std::cos(pitch)*std::sin(yaw) : std::sin(pitch);
    // TPS spells pick through the visible reticle from the shortened camera boom, excluding the actor itself.
    if(ThirdPerson()) {
        auto pose=thirdCamera_.Position(unit_,origin[2],ViewYaw(),ViewPitch(),CameraSettings(),hitboxes_,destructableHitboxes_);
        std::copy(pose.eye.begin(),pose.eye.end(),origin);
    }
}
SpellAim ShooterController::InventoryAim(int x,int y,int width,int height) {
    float origin[3],direction[3];AimRay(origin,direction);
    if(x>=0 && y>=0 && width>0 && height>0) {
        // Native projection plus the shared camera origin keeps free-cursor targets aligned in FPS and TPS.
        auto ray=CursorAimRay::Direction(ViewYaw(),ViewPitch(),inventoryVerticalFov_,float(x),float(y),float(width),float(height));
        std::copy(ray.begin(),ray.end(),direction);
    }
    return SpellAim::Pick(unit_,origin,direction,settings_.spells.aimDistance,hitboxes_,destructableHitboxes_);
}
void ShooterController::AimAndMove(float dt) {
    // A contained unit belongs to native cargo: changing XY/stance would fake an escape without releasing it.
    if (status_.contained || status_.hidden) { mouseLook_.Reset();movement_.Stop(); stepDistance_ = 0; return; }
    // Mouse capture occurs only in the foreground and only after the player enables FPS.
    float dx=0,dy=0;
    bool sampleMouse=!CursorMode();
#ifdef WCS_GAMEPLAY_TEST
    if(BackgroundGameplay::Enabled())sampleMouse=false;
#endif
    if (!sampleMouse) mouseLook_.Reset();else mouseLook_.Sample(dx,dy);
    if (!status_.incapacitated && !CursorMode()) {
        float sensitivity = scopeLevel_ == 2 ? 0.009f : Scoped() ? 0.035f : 0.14f;
        LookAngles::Apply(yaw_,pitch_,dx,dy,sensitivity);
    }
    MoveInput input;
    input.forward = float(Down(ShortcutAction::Forward)) - float(Down(ShortcutAction::Backward));
    input.right = float(Down(ShortcutAction::Right)) - float(Down(ShortcutAction::Left)); input.yaw = yaw_;
    input.walk = Down(ShortcutAction::Walk); input.duck = Down(ShortcutAction::Crouch); input.jump = Down(ShortcutAction::Jump);
    // Menu navigation stops horizontal input/look without pausing the Warcraft world or gravity.
    if (CursorMode()) { input.forward=input.right=0;input.jump=false;input.duck=movement_.Duck()>=.5f; }
    // Casting/channeling freezes manual movement while gravity and native unit placement still update.
    if(Casting()) {input.forward=input.right=0;input.jump=false;input.duck=movement_.Duck()>=.5f;}
    input.immobilized = status_.immobilized || Casting();
    input.stanceLocked = status_.incapacitated;
    if (status_.incapacitated) input.duck = movement_.Duck() >= 0.5f;
    static const float weaponSpeeds[WeaponSlots::Count] = {221, 230, 250, 210, 250, 250, 220};
    float speed = weapon_ == 3 && Scoped() ? 150.0f : weaponSpeeds[weapon_];
    // Apply Warcraft's current modifier to CS weapon speeds; roots stop movement but still allow falling.
    float speedScale=UnitScaling::MoveScale(settings_.movement.scaleFromUnitMoveSpeed,status_.nativeSpeed,
        settings_.movement.unitMoveSpeedReference,status_.speedScale);
    speed *= speedScale;
    // Third-person movement uses the hero's effective native speed, including Warcraft slows and haste.
    if(ThirdPerson() || UnitFirstPerson())speed=status_.nativeSpeed/MovementPhysics::worldScale;
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
    if (!status_.incapacitated && !Casting()) {
        float facing = LookAngles::Normalize(yaw_); wc3::SetUnitFacing(unit_, &facing);
    }
}
void ShooterController::UpdateStatus() {
    UnitStatus current = UnitStatus::Read(unit_, gameBase_);
    if (current.incapacitated != status_.incapacitated || current.immobilized != status_.immobilized ||
        current.disarmed != status_.disarmed || current.contained != status_.contained ||
        current.devoured != status_.devoured || current.hidden != status_.hidden || std::abs(current.speedScale - status_.speedScale) > 0.02f || std::abs(current.attackSpeed-status_.attackSpeed)>.02f) {
        wc3::Log("unit status stun=%d root=%d disarm=%d scale=%.3f native=%.1f default=%.1f stunCount=%d loaded=%d devoured=%d hidden=%d attackSpeed=%.3f hp=%.1f",
            current.incapacitated, current.immobilized, current.disarmed, current.speedScale,
            current.nativeSpeed, current.defaultSpeed, current.stunCount, current.contained, current.devoured, current.hidden,current.attackSpeed,
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
    // Lost attack ownership discards the pending TPS contact even on branches that return early.
    if(current.incapacitated || current.disarmed || current.contained || current.hidden)heroAttack_.Cancel();
    status_ = current;
}
ThirdPersonSettings ShooterController::CameraSettings() const {
    // Rendering and reticle rays must use exactly the same wheel-selected boom distance.
    auto settings=settings_.thirdPerson;settings.distance=thirdPersonDistance_;return settings;
}
void ShooterController::Camera() {
    // Keep native containment/cinematic positions intact; the overlay explains the unavailable world view.
    if (status_.contained || status_.hidden) return;
    // Presentation follows the same physical eye used by weapon rays.
    // Map RTS camera timers must not overwrite this pass; cinematics still receive control when suspended.
    ownCameraRequest_ = true;
    // Body/attachments are hidden by the world-only render filter; preserve native portrait tint/alpha.
    if(ThirdPerson()) {
        auto pose=thirdCamera_.Position(unit_,movement_.EyeZ()+wc3::Real(wc3::GetUnitFlyHeight(unit_)),
            yaw_,pitch_,CameraSettings(),hitboxes_,destructableHitboxes_);
        camera_.Update(pose.eye[0],pose.eye[1],pose.eye[2],yaw_,pitch_,85,settings_.render.drawDistance);
    } else camera_.Update(wc3::Real(wc3::GetUnitX(unit_)), wc3::Real(wc3::GetUnitY(unit_)),
            movement_.EyeZ() + wc3::Real(wc3::GetUnitFlyHeight(unit_)), yaw_ + recoil_.Yaw(), std::clamp(pitch_ + recoil_.Pitch(), -65.0f, 65.0f),
            scopeLevel_ == 2 ? 10.0f : Scoped() ? 40.0f : 85.0f, settings_.render.drawDistance);
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
    if(settings_.warcraftOnly)return;
    // F11 restores carried ammo and grants C4; F12 additionally unlocks every weapon without spending gold.
    int magazines[WeaponSlots::Count],reserves[WeaponSlots::Count];
    for (int i=0;i<WeaponSlots::Count;++i) { magazines[i]=weapons[i].magazine;reserves[i]=settings_.maxReserve[i]; }
    inventory_.Refill(settings_,ammo_,reserve_,magazines,reserves,grantAll);
    if (reloadEnd_) { reloadEnd_ = 0; audio_.CancelAnimation(); Play("idle"); }
    CancelPlant();ammoRecovery_.Reset();
    Notice(grantAll ? "ALL WEAPONS AND AMMO GRANTED" : "ALL AMMO RESTORED");
    wc3::Log("%s free inventory: AK47=%d/%d M4A1=%d/%d USP=%d/%d AWP=%d/%d C4=%d gold=%d",
        Shortcut(grantAll ? ShortcutAction::AllWeapons : ShortcutAction::Refill).c_str(),ammo_[0],reserve_[0],ammo_[1],reserve_[1],ammo_[2],reserve_[2],ammo_[3],reserve_[3],ammo_[5],gold_);
}
void ShooterController::SwitchWeapon(int slot) {
    if (slot<0 || slot>=WeaponSlots::Count || !inventory_.owned[slot] || weapon_ == slot) return;
    // Number keys and the wheel share deployment: cancel reload, scope, planting and pending melee cues.
    CancelPlant(); audio_.CancelAnimation(); meleeContact_ = meleeReady_ = 0;
    recoil_.ResetBurst(); weapon_ = slot; reloadEnd_ = 0; scopeLevel_ = 0; Play("draw");
    wc3::Log("weapon selected=%s slot=%d", weapons[slot].name, slot + 1);
}
void ShooterController::ToggleHealthBars() {
    settings_.healthBars.ToggleAll();
    // Write both compatible keys even if one save fails; leave unrelated preferences intact.
    const char* value=settings_.healthBars.allies ? "true" : "false";
    std::string path=root_+"\\WarcraftCS.ini";
    bool alliesSaved=WritePrivateProfileStringA("Interface","ShowAllyHealthBars",value,path.c_str())!=FALSE;
    bool enemiesSaved=WritePrivateProfileStringA("Interface","ShowEnemyHealthBars",value,path.c_str())!=FALSE;
    bool saved=alliesSaved && enemiesSaved;
    wc3::Log("Health bars toggled enabled=%d alliesSaved=%d enemiesSaved=%d",settings_.healthBars.allies,alliesSaved,enemiesSaved);
    Notice(saved ? (settings_.healthBars.allies ? "HEALTH BARS ON" : "HEALTH BARS OFF") : "HEALTH BAR SETTINGS COULD NOT BE SAVED");
}
void ShooterController::ReloadSettings() {
    // Replace one complete snapshot; stale fractional credit must not survive a percentage/scope change.
    settings_=GameplaySettings::Load(root_+"\\WarcraftCS.ini");
    // Startup/F8 restore the configured distance; wheel choices stay in this map session only.
    thirdPersonDistance_=settings_.thirdPerson.distance;cameraWheel_.Reset();
    // Reducing a cap on F8 discards surplus reserve, but cannot grant ammunition or empty the magazine.
    for(int slot=0;slot<4;++slot)reserve_[slot]=std::min(reserve_[slot],settings_.maxReserve[slot]);
    wc3::Log("Unit scaling fireRate=%d moveSpeed=%d moveReference=%.1f",settings_.scaleFireRateFromAttackSpeed,
        settings_.movement.scaleFromUnitMoveSpeed,settings_.movement.unitMoveSpeedReference);
    itemInventory_.ApplySettings(settings_); // Startup/F8 discard gestures and restore the configured Tab default.
    squadOrderMode_.Configure(settings_.squadReticleAttackInFPS);
    squadRequested_=0;if(!SquadReticleOrdersEnabled())squad_.CancelFocus(); // F8 restores the initial F6 mode and releases opt-outs.
    if(!settings_.inventoryControls)itemInventory_.CancelCasting();
    if(!settings_.spells.enabled)spells_.Cancel(); // Live disabling releases an explicitly owned cast.
    wc3::Log("Health bars configured allies=%d enemies=%d distance=%.0f",settings_.healthBars.allies,settings_.healthBars.enemies,settings_.healthBars.distance);
    // Camera and runtime pacing consume this same snapshot on the next render frame.
    wc3::Log("Rendering configured maxFPS=%d drawDistance=%.0f",settings_.render.maxFPS,settings_.render.drawDistance);
    // Reload restores saved preferences and the complete keyboard layout without replaying held action edges.
    std::memset(keys_,0,sizeof(keys_));spellRequested_=-1;spellCancelRequested_=false;
    for(size_t i=0;i<ShortcutBindings::Count;++i) {
        auto action=ShortcutAction(i);int key=settings_.shortcuts.Key(action);
        if(key>0)keys_[key]=Down(key);
        wc3::Log("Shortcut %s=%s",ShortcutBindings::Name(action),Shortcut(action).c_str());
    }
    if(ThirdPerson() && !settings_.thirdPerson.enabled) {
        if(settings_.warcraftOnly && !settings_.unitViewEnabled)Disable(true);
        else ToggleView();
    }
    if(UnitFirstPerson() && !settings_.unitViewEnabled)Disable(true);
    // F8 may tighten the installation profile, releasing weapon/UI work through the normal transition.
    if(settings_.warcraftOnly) {
        settings_.csSky=false;audio_.Stop();
        if(Visible() && view_==ControlView::WeaponsFirstPerson) {
            if(settings_.thirdPerson.enabled)SelectView(ControlView::ThirdPerson);
            else Disable(true);
        }
        buyMenu_.Close();
    }
    wc3::Log("Third person configured enabled=%d distance=%.1f range=%.1f arc=%.1f interval=%.2f delay=%.2f multiplier=%.2f",
        settings_.thirdPerson.enabled,settings_.thirdPerson.distance,settings_.thirdPerson.attackRange,settings_.thirdPerson.attackArc,
        settings_.thirdPerson.attackInterval,settings_.thirdPerson.contactDelay,settings_.thirdPerson.damageMultiplier);
    wc3::Log("Spells configured enabled=%d aimDistance=%.0f",settings_.spells.enabled,settings_.spells.aimDistance);
    for(int slot=0;slot<4;++slot)wc3::Log("Spell slot=%d ability=%s order=%d target=%s",slot+1,settings_.spells.ability[slot].c_str(),settings_.spells.order[slot],settings_.spells.target[slot].c_str());
    showSpeed_=settings_.showSpeed;
    showMinimap_=settings_.showMinimap;
    wc3::Log("Minimap configured visible=%d sizePercent=%.1f",showMinimap_,settings_.minimapSizePercent);
    fogOfWarDisabled_=settings_.disableFogOfWar;
    wc3::Log("Fog of war configured disabled=%d",fogOfWarDisabled_);
    wc3::Log("Speed display configured visible=%d units=CS",showSpeed_);
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
    // Distinguish a stored preference from the finishing rule actually active in this damage mode.
    wc3::Log("Attack effects enabled=%d AWP finishing active=%d",settings_.enableAttackEffects,settings_.FinishingAWP(WeaponSlots::AWP));
    wc3::Log("Pass-through effect objects enabled=%d",settings_.ignorePassableInvulnerableObjects);
    // Log the validated snapshot, including prices/sky choices, rather than arbitrary raw INI contents.
    wc3::Trace("settings loadoutAll=%d bombs=%d/%d unlimitedBombPurchases=%d buyAccess=%d buyRadius=%.1f shops=%s csVolume=%.1f floatingText=%.1f squadRadius=%.1f follow=%.1f leash=%.1f maxUnits=%d csSky=%d nativeSky=%d defaultSky=%s",
        settings_.startAllWeapons, settings_.startBombs, settings_.maxBombs,settings_.unlimitedBombPurchases,int(settings_.buyAccess), settings_.buyRadius,
        settings_.shopTypes.c_str(), settings_.csVolumePercent, settings_.floatingTextDistance, settings_.squadRadius,
        settings_.squadFollowDistance, settings_.squadLeash, settings_.squadMaxUnits, settings_.csSky, settings_.nativeSky, settings_.defaultSky.c_str());
    for (int slot = 0; slot < WeaponSlots::Count; ++slot)
        wc3::Trace("settings weapon=%s damage=%.1f heroMultiplier=%.2f price=%d ammoPrice=%d ammoPack=%d maxReserve=%d",
            weapons[slot].name, settings_.damage[slot], settings_.heroMultiplier[slot], settings_.weaponPrice[slot], settings_.ammoPrice[slot], settings_.ammoPack[slot],settings_.maxReserve[slot]);
    wc3::Trace("settings secondaryDamage knife=%.1f sword=%.1f", settings_.knifeSecondaryDamage, settings_.swordSecondaryDamage);
    for (unsigned code = 0; code < settings_.tilesetSky.size(); ++code)
        if (!settings_.tilesetSky[code].empty()) wc3::Trace("settings tileset=%c sky=%s", code, settings_.tilesetSky[code].c_str());
}
void ShooterController::PickupItem() {
    if (status_.incapacitated || status_.contained || status_.hidden) return;
    ItemPickupResult result=itemPickup_.Take(unit_,settings_.runePickupRadius);
    if (!result.found) return;
    // Equipment uses native inventory slots; only successful powerups award the configured rune ammo.
    if (!result.accepted || !result.powerup || settings_.warcraftOnly) {
        ammoMessage_=result.accepted ? "ITEM PICKED UP" : "CANNOT PICK UP ITEM";
        refillTick_=GetTickCount();return;
    }
    // Refill configured magazines and reserves without exceeding their capacities; C4 uses fractional credit.
    int added=0;
    for (int slot=0;slot<WeaponSlots::Count;++slot) {
        if (!inventory_.owned[slot]) continue;
        if (!settings_.runeAmmoAllWeapons && slot!=weapon_) continue;
        added+=ammoRecovery_.Restore(slot,settings_.runeAmmoPercent,slot==WeaponSlots::C4 ? settings_.maxBombs : weapons[slot].magazine,settings_.maxReserve[slot],ammo_[slot],reserve_[slot]);
        wc3::Log("rune ammo weapon=%s ammo=%d reserve=%d",weapons[slot].name,ammo_[slot],reserve_[slot]);
    }
    char message[96];sprintf_s(message,"RUNE: +%.0f%% %s AMMO",settings_.runeAmmoPercent,settings_.runeAmmoAllWeapons ? "ALL" : "CURRENT");
    ammoMessage_=message;refillTick_=GetTickCount();itemNearby_=false;
    wc3::Log("rune ammo reward added=%d",added);
}
void ShooterController::Fire(bool secondary) {
    if(settings_.warcraftOnly)return;
    const Weapon& w = weapons[weapon_]; DWORD now = GetTickCount();
    // Keep CS's fractional firing interval: truncating 87.5ms would permit an early M4 shot.
    // IAS changes firearm cadence only; planting and authored melee recovery keep their existing timing.
    float interval=UnitScaling::FireInterval(w.interval,weapon_<4 && settings_.scaleFireRateFromAttackSpeed,status_.attackSpeed);
    if (reloadEnd_ || now < meleeReady_ || float(now - lastShot_) < interval * 1000) return;
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
    if(UnitFirstPerson() || settings_.warcraftOnly)return; // Queued contacts cannot bypass the installation profile.
    const Weapon& w = weapons[weapon_];

    float x = wc3::Real(wc3::GetUnitX(unit_)), y = wc3::Real(wc3::GetUnitY(unit_));
    // Shots originate at the same eye used by the camera, including airborne/crouched poses.
    float z = movement_.EyeZ() + wc3::Real(wc3::GetUnitFlyHeight(unit_));
    float yaw = (yaw_ + recoil_.Yaw()) * radians, pitch = std::clamp(pitch_ + recoil_.Pitch(), -65.0f, 65.0f) * radians;
    float vx = std::cos(pitch) * std::cos(yaw), vy = std::cos(pitch) * std::sin(yaw), vz = std::sin(pitch);
    float attackRange = WeaponSlots::Melee(weapon_) ? meleeAttack_.range : w.range;
    float nearest = attackRange;
    // Solid terrain/decks stop shots; liquid alone must not shield the visible submerged part of a unit.
    for (float t = 48; t < nearest; t += 48) {
        if (wc3::AttackSurface(x + vx * t, y + vy * t,settings_.waterBlocksAttacks) > z + vz * t) { nearest = t; break; }
    }
    wc3::Handle group = wc3::CreateGroup(), target = 0, candidate = 0, local = wc3::GetLocalPlayer();
    // Include models whose surface enters range before their center.
    float radius = attackRange + 512; wc3::GroupEnumUnitsInRange(group, &x, &y, &radius, 0);
    while ((candidate = wc3::FirstOfGroup(group)) != 0) {
        wc3::GroupRemoveUnit(group, candidate);
        // Disabled allied targeting makes allies transparent before choosing the nearest weapon contact.
        if (candidate == unit_ || wc3::Real(wc3::GetUnitState(candidate, 0)) <= 0.405f ||
            !wc3::IsUnitVisible(candidate, local)) continue;
        if(!settings_.targetAllies && wc3::IsUnitAlly(candidate,wc3::GetOwningPlayer(unit_)))continue;
        // Intersect the rendered model's oriented 3D volume instead of one fixed ground cylinder.
        float origin[] = {x, y, z}, direction[] = {vx, vy, vz}, entry = 0;
        float padding = weapon_ == WeaponSlots::Sword ? 28.0f : 0;
        if (hitboxes_.Intersect(candidate, origin, direction, nearest, entry, padding)) {
            // Skip native nonphysical effects without shortening the ray before the enemy behind them.
            if(settings_.ignorePassableInvulnerableObjects && WeaponTargetFilter::PassThrough(candidate,gameBase_)) {
                wc3::Log("shot pass-through unit=%08X type=%08X entry=%.1f",candidate,wc3::GetUnitTypeId(candidate),entry);
                continue;
            }
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
        if(!destructable && !settings_.targetAllies && wc3::IsUnitAlly(target,wc3::GetOwningPlayer(unit_)))return;
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
        // Confirmed weapon hits dispatch one native attack, including the map's attack effects, without pursuit.
        // C4/destructables and explicit finishing retain their own policies; zero friendly damage causes no events.
        // Native impact compares unit centers; the ray has already enforced the weapon's surface range.
        bool effects=damage>0 && !destructable && !lethal && settings_.enableAttackEffects &&
            NativeWeaponAttack::Apply(unit_,target,damage,std::hypot(wc3::Real(wc3::GetUnitX(target))-x,wc3::Real(wc3::GetUnitY(target))-y));
        if(damage>0 && !effects)wc3::UnitDamageTarget(unit_,target,&damage,FALSE,TRUE,
            destructable || lethal ? 0 : NativeWeaponAttack::AttackType(unit_),lethal ? 26 : 4,0);
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
        wc3::Log("shot weapon=%s target=%08X damage=%.1f hpBefore=%.1f hpAfter=%.1f allied=%d destructable=%d finishing=%d effects=%d", w.name, target, damage, before, after,allied,destructable,lethal,effects);
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
    bool moving = Down(ShortcutAction::Forward) || Down(ShortcutAction::Left) || Down(ShortcutAction::Backward) || Down(ShortcutAction::Right) || Down(ShortcutAction::Jump);
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
// Squad tactics are shared by armed views and the unarmed unit camera.
void ShooterController::UpdateSquad(DWORD now) {
    if (squadRequested_) {
        // Both keys recruit nearby owned units and change the existing squad's strategy as one operation.
        if (!status_.incapacitated && !status_.contained && !status_.hidden) {
            if(squadRequested_==4) {
                if(SquadReticleOrdersEnabled() && !Buying() && !InventoryOpen()) {
                    float origin[3],direction[3];AimRay(origin,direction);
                    auto aim=SpellAim::Pick(unit_,origin,direction,settings_.squadReticleDistance,hitboxes_,destructableHitboxes_);
                    size_t count=squad_.Focus(unit_,aim.target,settings_,now);
                    char message[96];sprintf_s(message,count ? "ALL UNITS: ATTACK TARGET | %u" : "TARGET ORDER CLEARED",unsigned(count));
                    Notice(message);wc3::Log("squad reticle target=%08X commanded=%u hp=%.1f",aim.target,unsigned(count),
                        count ? wc3::Real(wc3::GetUnitState(aim.target,0)) : 0.f);
                }
            } else {
                squad_.Capture(unit_,squadRequested_==2,settings_);
                char message[96];sprintf_s(message,"SQUAD: %u | %s",unsigned(squad_.Count()),squad_.Passive() ? "FOLLOW ONLY" : "FOLLOW + FIGHT");
                ammoMessage_=message;refillTick_=now;
            }
        }
        squadRequested_=0;
    }
    squad_.Tick(unit_,settings_,now,!status_.contained && !status_.hidden,yaw_,movement_.VelocityX(),movement_.VelocityY());
}
void ShooterController::Tick(uintptr_t ui) {
    aimTarget_={}; // Clear before every early return, mode change, menu or lost-control state.
    if (ui_ && ui != ui_) ResetMap(); ui_ = ui;
    DWORD now = GetTickCount();
    // High-resolution elapsed time smooths movement at the new cap without speeding up gameplay timers.
    auto frameTime=std::chrono::steady_clock::now();
    float dt=tick_.time_since_epoch().count() ? std::min(std::chrono::duration<float>(frameTime-tick_).count(),0.05f) : 0;
    tick_=frameTime;
    DWORD foregroundPid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
    if (bombs_.Tick(audio_) && Visible()) hits_.Record();
    bool requireForeground=true;
#ifdef WCS_GAMEPLAY_TEST
    requireForeground=!BackgroundGameplay::Enabled();
#endif
    if (requireForeground && foregroundPid != GetCurrentProcessId()) {
        thirdPersonRequested_=unitViewRequested_=false; // A background tap cannot acquire native control on refocus.
        unitLocomotion_.Release();
        // Drop background packets; refocusing establishes a fresh cursor anchor without a view jump.
        mouseLook_.Reset();
        weaponWheel_.Reset();cameraWheel_.Reset();
        itemRequested_=false;squadRequested_=0;squadOrderMode_.ClearInput(); // Commands pressed before losing focus must not execute on return.
        buyMenu_.Close();itemInventory_.ClearInput();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;
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
    if (suspended_) { unitLocomotion_.Release();heroAttack_.Cancel();viewToggleRequested_=false;CancelCasting();spellRequested_=-1;spellCancelRequested_=false;CancelPlant(); meleeContact_ = 0;buyMenu_.Close();itemInventory_.ClearInput();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1; }
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
    // F7 enters directly from RTS, selects TPS from FPS, and returns TPS to RTS; F6 recovery is unchanged.
    if(thirdPersonRequested_) {
        thirdPersonRequested_=false;
        if(active_ && ThirdPerson())Disable(true);
        else if(settings_.thirdPerson.enabled && !paused_ && !*reinterpret_cast<int*>(ui+0x258) && !*reinterpret_cast<int*>(ui+0x260)) {
            if(!active_) {cinematicRequested_=false;suspended_=false;Toggle(ControlView::ThirdPerson);}
            if(Visible() && !ThirdPerson())ToggleView();
        }
    }
    if(unitViewRequested_) {
        unitViewRequested_=false;
        if(active_ && UnitFirstPerson())Disable(true);
        else if(settings_.unitViewEnabled && !paused_ && !*reinterpret_cast<int*>(ui+0x258) && !*reinterpret_cast<int*>(ui+0x260)) {
            if(!active_){cinematicRequested_=false;suspended_=false;Toggle(ControlView::UnitFirstPerson);}
            if(Visible())SelectView(ControlView::UnitFirstPerson);
        }
    }
    // Menus/cinematics return ownership to the map; live FPS may temporarily fill a missing native sky.
    nativeSky_.Update(ui_,Visible() && settings_.nativeSky && !settings_.csSky,mapTileset_);
    nativeFog_.Update(Visible() && fogOfWarDisabled_);
    // A queued display tap cannot take effect after FPS is suspended or left.
    if (!active_ || suspended_) { itemInventory_.ClearInput();squadOrderMode_.ClearInput();mouseLook_.Reset();weaponWheel_.Reset();cameraWheel_.Reset();itemRequested_=false;squadRequested_=0;speedToggleRequested_=fogToggleRequested_=minimapToggleRequested_=healthBarToggleRequested_=allyTargetToggleRequested_=false;spellRequested_=-1;spellCancelRequested_=false;return; }
    if (settingsRequested_) { settingsRequested_=false;ReloadSettings();if(!Visible())return; } // Unit-view opt-out may return to RTS.
    if(viewToggleRequested_) {viewToggleRequested_=false;ToggleView();}
    if(healthBarToggleRequested_) { healthBarToggleRequested_=false;ToggleHealthBars(); }
    if(allyTargetToggleRequested_) {
        // Apply before both prediction and contact so Q changes the marker and damage policy together.
        allyTargetToggleRequested_=false;settings_.targetAllies=!settings_.targetAllies;aimTarget_={};
        ammoMessage_=settings_.targetAllies ? "ALLY TARGETING ON" : "ALLY TARGETING OFF";refillTick_=now;
        wc3::Log("Ally targeting enabled=%d",settings_.targetAllies);
    }
    if (speedToggleRequested_) {
        speedToggleRequested_=false;showSpeed_=!showSpeed_;
        wc3::Log("Speed display toggled visible=%d units=CS",showSpeed_);
    }
    // One M press changes visibility without editing the player's persisted configuration.
    if (minimapToggleRequested_) {
        minimapToggleRequested_=false;showMinimap_=!showMinimap_;
        wc3::Log("Minimap toggled visible=%d",showMinimap_);
    }
    if (fogToggleRequested_) {
        fogToggleRequested_=false;fogOfWarDisabled_=!fogOfWarDisabled_;
        wc3::Log("Fog of war toggled disabled=%d",fogOfWarDisabled_);
    }
    // Apply N/F8 immediately, including restoration when the override is switched off.
    nativeFog_.Update(fogOfWarDisabled_);
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
                    pitch_=std::atan2(z-movement_.EyeZ()-wc3::Real(wc3::GetUnitFlyHeight(unit_))-(ThirdPerson() ? settings_.thirdPerson.height : 0),std::hypot(dx,dy))/radians;
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
                    pitch_ = std::atan2(z - movement_.EyeZ()-(ThirdPerson() ? settings_.thirdPerson.height : 0), distance) / radians;
                }
                wc3::Log("verification target=%08X type=%08X at=%.1f,%.1f fly=%.1f side=%.1f height=%.1f scale=%.1f ownerSlot=%d neutral=%d",
                    fixtureTarget_, type, x, y, wc3::Real(wc3::GetUnitFlyHeight(fixtureTarget_)), side, aimHeight, scale, fixtureOwner, wc3::IsNeutralUnit(fixtureTarget_));
            }
        }
    }
    bool escape=Pressed(VK_ESCAPE);
    if(inventoryEscape_) {escape=false;inventoryEscape_=false;}
    if(escape && InventoryOpen()) {itemInventory_.ClearInput();mouseLook_.Reset();suppressFire_=true;escape=false;}
    if (escape && Buying()) { buyMenu_.Close();mouseLook_.Reset();suppressFire_=true;escape=false; }
    if (escape || !wc3::GetUnitTypeId(unit_) || wc3::Real(wc3::GetUnitState(unit_, 0)) <= 0.405f) { Disable(true); return; }
    health_ = wc3::Real(wc3::GetUnitState(unit_, 0));
    // Copy HUD state on the simulation thread; no native unit/item queries occur during GL presentation.
    if(!UnitFirstPerson() && (settings_.showUnitHud || settings_.showInventory || InventoryOpen() || ItemControls().Numbers()) && now-unitHudTick_>=100) {
        unitHudTick_=now;unitHud_=unitHudReader_.Read(unit_,settings_.showInventory || InventoryOpen() || ItemControls().Numbers());
    }
    UpdateStatus();
    // Apply queued ; before RMB dispatch. Held buttons cannot turn a mode change into an accidental scope/stab.
    if(squadOrderMode_.Apply(settings_.squadReticleAttack && !ThirdPerson() && !UnitFirstPerson() && !Buying() &&
        !InventoryInputOpen() && !status_.incapacitated && !status_.contained && !status_.hidden)) {
        scopeLevel_=0;keys_[VK_RBUTTON]=Down(VK_RBUTTON);suppressFire_=true;
        if(!squadOrderMode_.Enabled())squad_.CancelFocus();
        Notice(squadOrderMode_.Enabled() ? "RMB: ALL-UNIT TARGET ORDERS" : "RMB: WEAPON ACTION");
        wc3::Log("squad F6 right-click orders enabled=%d",squadOrderMode_.Enabled());
    }
    bool wasInventory=InventoryOpen();
    auto inventoryNotice=itemInventory_.Update(unit_,settings_,UnitFirstPerson() || status_.incapacitated || status_.contained || status_.hidden,yaw_,Down(ShortcutAction::SelfCast),
        [&](int x,int y,int w,int h){return InventoryAim(x,y,w,h);},[&] {
            CancelCasting();heroAttack_.Cancel();unitLocomotion_.Release();movement_.Stop();
            CancelPlant();meleeContact_=reloadEnd_=0;suppressFire_=true;
        });
    if(inventoryNotice) {Notice(inventoryNotice);unitHudTick_=0;}
    if(wasInventory!=InventoryOpen()) {
        mouseLook_.Reset();weaponWheel_.Reset();cameraWheel_.Reset();suppressFire_=true;
        if(InventoryOpen())buyMenu_.Close();
    }
    // HUD sampling runs on the game thread and never stores native pointers for the GL overlay.
    if(settings_.showSpellIcons && now-spellHudTick_>=100) {
        spellHudTick_=now;spellHud_=spells_.Snapshot(unit_,settings_.spells);
        if(DiagnosticLog::Due(spellHudLog_))for(int i=0;i<4;++i) {
            const auto& slot=spellHud_[i];wc3::Trace("spell HUD slot=%d ability=%08X level=%d passive=%d cooldownKnown=%d remaining=%.2f total=%.2f command=%d noMana=%d disabled=%d charges=%d icon=%s",
                i+1,slot.ability,slot.level,slot.passive,slot.cooldownKnown,slot.cooldown,slot.cooldownTotal,slot.commandKnown,slot.noMana,slot.disabled,slot.charges,slot.icon.c_str());
        }
    }
    // Releasing followers returns native control even when their FPS leader cannot act.
    if (squadRequested_==3) {
        squad_.Release();squadRequested_=0;ammoMessage_="SQUAD RELEASED";refillTick_=now;
        wc3::Log("squad released by shortcut: count=%u",unsigned(squad_.Count()));
    }
    recoil_.Step(dt, Down(VK_LBUTTON) && !status_.incapacitated && !status_.disarmed, weapon_);
    if (status_.incapacitated) {
        unitLocomotion_.Release(); // Early disabled-state returns must not retain the running time scale.
        CancelCasting();spellRequested_=-1;spellCancelRequested_=false;
        buyMenu_.Close();itemInventory_.ClearInput();buyToggleRequested_=buyAmmoRequested_=false;buyKeyRequested_=-1;suppressFire_=true;
        // Discard scrolling during disabled states rather than replaying a switch after recovery.
        weaponWheel_.Reset();cameraWheel_.Reset();
        // Disabled leaders cannot queue pickup/recruitment for later; existing followers retain their chosen policy.
        itemRequested_=false;squadRequested_=0;
        squad_.Tick(unit_,settings_,now,!status_.contained && !status_.hidden,yaw_,movement_.VelocityX(),movement_.VelocityY());
        // Consume action edges while disabled, so held reload/scope/switch inputs are not replayed on recovery.
        for(int i=0;i<WeaponSlots::Count;++i)Pressed(ShortcutAction(int(ShortcutAction::Weapon1)+i));
        Pressed(ShortcutAction::Reload);Pressed(VK_LBUTTON);Pressed(VK_RBUTTON);
        refillRequested_ = false; allWeaponsRequested_=false;
        AimAndMove(dt); Camera(); audio_.Tick(GetTickCount());
        return;
    }
    if(UnitFirstPerson()) {
        // The view grants movement only, without changing vulnerability or modifying native attack statistics.
        CancelCasting();heroAttack_.Cancel();CancelPlant();meleeContact_=0;reloadEnd_=0;
        refillRequested_=allWeaponsRequested_=itemRequested_=buyToggleRequested_=buyAmmoRequested_=false;
        buyKeyRequested_=spellRequested_=-1;spellCancelRequested_=false;buyMenu_.Close();
        FpsCombatGuard::CancelOrders(unit_);UpdateSquad(now);
        AimAndMove(dt);Camera();audio_.Tick(now);return;
    }
    // Drop CS input mailboxes before the shared TPS attack/spell path in reduced installations.
    if(settings_.warcraftOnly) {
        refillRequested_=allWeaponsRequested_=buyToggleRequested_=buyAmmoRequested_=false;
        buyKeyRequested_=-1;buyMenu_.Close();
    }
    if (allWeaponsRequested_) { allWeaponsRequested_=false;refillRequested_=false;RefillAmmo(true); }
    if (refillRequested_) { refillRequested_ = false; RefillAmmo(); }
    UpdateBuyMenu();
    if (!Down(VK_LBUTTON) && !Down(VK_RBUTTON)) suppressFire_=false;
    if (Buying()) {
        unitLocomotion_.Release();
        heroAttack_.Cancel();
        CancelCasting();spellRequested_=-1;spellCancelRequested_=false;
        itemRequested_=false;squadRequested_=0; // Menu taps cannot queue RTS interaction for after closing.
        for (int key='0';key<='9';++key) Pressed(key);
        Pressed(VK_LBUTTON);Pressed(VK_RBUTTON);Pressed(ShortcutAction::Reload);
        if (!status_.contained && !status_.hidden) FpsCombatGuard::CancelOrders(unit_);
        squad_.Tick(unit_,settings_,now,!status_.contained && !status_.hidden,yaw_,movement_.VelocityX(),movement_.VelocityY());
        movement_.Stop();weaponWheel_.Reset();cameraWheel_.Reset();AimAndMove(dt);Camera();audio_.Tick(now);return;
    }
    if(InventoryOpen()) {
        // Native item channels continue while the panel owns pointer/movement; clicks cannot become attacks.
        spellRequested_=-1;spellCancelRequested_=false;itemRequested_=false;squadRequested_=0;
        if(!Casting())FpsCombatGuard::CancelOrders(unit_);
        for(int key='0';key<='9';++key)Pressed(key);
        Pressed(VK_LBUTTON);Pressed(VK_RBUTTON);Pressed(ShortcutAction::Reload);
        movement_.Stop();weaponWheel_.Reset();cameraWheel_.Reset();UpdateSquad(now);
        AimAndMove(dt);Camera();audio_.Tick(now);return;
    }
    // TPS zoom and FPS weapon selection consume separate queues; scrolling never crosses view modes.
    if(ThirdPerson()) {
        float distance=cameraWheel_.Take(thirdPersonDistance_,settings_.thirdPerson);
        if(distance!=thirdPersonDistance_)wc3::Log("Third-person camera distance=%.1f",distance);
        thirdPersonDistance_=distance;
    }
    int wheel=weaponWheel_.Take(weapon_);
    int steps=(wheel-weapon_+WeaponSlots::Count)%WeaponSlots::Count;
    if (steps>WeaponSlots::Count/2) steps-=WeaponSlots::Count;
    if(!ThirdPerson())SwitchWeapon(inventory_.Next(weapon_,steps));
    for (int i = 0; i < WeaponSlots::Count; ++i) if (Pressed(ShortcutAction(int(ShortcutAction::Weapon1)+i)) && !ThirdPerson() &&
        InventoryControls::NumberSlot(settings_.shortcuts.Key(ShortcutAction(int(ShortcutAction::Weapon1)+i)),ItemControls().Numbers(),false)<0) SwitchWeapon(i);
    if (Pressed(ShortcutAction::Reload) && !ThirdPerson()) Reload();
    // Consume the right-click edge for every weapon so holding it across a switch never scopes or stabs.
    // Consume the physical edge even when RMB belongs to the squad, preventing a deferred scope/stab.
    bool secondaryPressed = Pressed(VK_RBUTTON) && !SquadReticleOrdersEnabled();
    if (!ThirdPerson() && !suppressFire_ && weapon_ == 3 && secondaryPressed) {
        // CS-style right-click cycle: normal view, first zoom, second zoom, normal view.
        scopeLevel_ = (scopeLevel_ + 1) % 3;
        wc3::Log("AWP scope level=%d fov=%.0f", scopeLevel_, scopeLevel_ == 2 ? 10.0f : Scoped() ? 40.0f : 85.0f);
    }
    if (reloadEnd_ && now >= reloadEnd_) {
        int amount = std::min(weapons[weapon_].magazine - ammo_[weapon_], reserve_[weapon_]);
        ammo_[weapon_] += amount; reserve_[weapon_] -= amount; reloadEnd_ = 0; Play("idle");
        wc3::Log("reload complete: weapon=%s ammo=%d reserve=%d", weapons[weapon_].name, ammo_[weapon_], reserve_[weapon_]);
    }
    // Preserve only our explicit native spell order; normal AI cancellation resumes when it ends.
    spells_.Update();
    if(spellCancelRequested_) {spellCancelRequested_=false;CancelCasting();}
    if(spellRequested_>=0) {
        int slot=spellRequested_;spellRequested_=-1;
        if(!status_.contained && !status_.hidden) {
            float origin[3],direction[3];AimRay(origin,direction);
            auto aim=SpellAim::Pick(unit_,origin,direction,settings_.spells.aimDistance,hitboxes_,destructableHitboxes_);
            Notice(spells_.Cast(unit_,slot,settings_.spells,aim,Down(ShortcutAction::SelfCast)));
            if(Casting()) {heroAttack_.Cancel();movement_.Stop();CancelPlant();meleeContact_=0;reloadEnd_=0;suppressFire_=true;}
        }
    }
    // Native combat/movement guards still block AI while a manual clip owns the pose. Defer Stop:
    // repeated crowd acquisitions otherwise enqueue Stand and interrupt the player's backswing.
    if (!Casting() && !status_.incapacitated && !status_.contained && !status_.hidden &&
        !heroAttack_.Animating(now*.001)) FpsCombatGuard::CancelOrders(unit_);
    if (itemRequested_) { itemRequested_=false;PickupItem(); }
    UpdateSquad(now);
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
    if(ThirdPerson()) {
        // Restore locomotion's temporary rate before playing a native attack. Held LMB respects IAS-scaled recovery.
        if(!Casting() && !suppressFire_ && Down(VK_LBUTTON) && !heroAttack_.Busy(now*.001) && !heroAttack_.Animating(now*.001)) {
            unitLocomotion_.Release();heroAttack_.Begin(unit_,now*.001,settings_,status_);
        }
        // Reuse the real reticle only at contact; idle/recovery frames need no extra camera collision enumeration.
        float attackOrigin[3]={},attackDirection[3]={};if(heroAttack_.ContactDue(now*.001))AimRay(attackOrigin,attackDirection);
        if(heroAttack_.Tick(unit_,now*.001,yaw_,movement_.FeetZ()+wc3::Real(wc3::GetUnitFlyHeight(unit_)),attackOrigin,attackDirection,
            Casting(),settings_,status_,hitboxes_,destructableHitboxes_))hits_.Record();
        // Native casts/stuns and the authored swing clip own the pose, while DPS recovery no longer freezes running.
        unitLocomotion_.Update(unit_,now*.001,movement_.Grounded(),movement_.Speed(),
            Casting() || status_.incapacitated || status_.contained || status_.hidden || heroAttack_.Animating(now*.001),settings_.thirdPerson,Down(ShortcutAction::Walk) || movement_.Duck()>.5f);
        // A separate read-only snapshot advertises the same contact without starting or advancing an attack.
        if(settings_.thirdPerson.showTargetIndicator && !Casting() && !status_.incapacitated && !status_.disarmed &&
            !status_.contained && !status_.hidden) {
            float origin[3],direction[3];AimRay(origin,direction);
            aimTarget_=heroAttack_.Preview(unit_,yaw_,movement_.FeetZ()+wc3::Real(wc3::GetUnitFlyHeight(unit_)),origin,direction,
                settings_,hitboxes_,destructableHitboxes_);
        }
    } else if (!Casting() && !suppressFire_ && weapon_ == WeaponSlots::C4) PlantC4(dt);
    else if (!Casting() && !suppressFire_ && !status_.disarmed && WeaponSlots::Melee(weapon_) && secondaryPressed) Fire(true);
    else if (!Casting() && !suppressFire_ && !status_.disarmed && ((weapons[weapon_].automatic && Down(VK_LBUTTON)) || firePressed)) Fire();
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
