#pragma once
#include "../combat/ShotgunReload.hpp"
#include "../combat/BurstFire.hpp"
#include "../presentation/ViewmodelMotion.hpp"
#include "../combat/Weapon.hpp"
#include "../combat/WeaponSlots.hpp"
#include "../platform/WarcraftApi.hpp"
#include "../movement/MovementPhysics.hpp"
#include "../platform/WarcraftCollision.hpp"
#include "../presentation/FirstPersonCamera.hpp"
#include "../presentation/ThirdPersonCamera.hpp"
#include "../platform/UnitAttack.hpp"
#include "../platform/UnitLocomotion.hpp"
#include "../platform/NativeSky.hpp"
#include "../platform/NativeFogOfWar.hpp"
#include "../audio/GameAudio.hpp"
#include "../presentation/HitFeedback.hpp"
#include "../combat/UnitStatus.hpp"
#include "../combat/UnitHitboxes.hpp"
#include "../combat/DestructableHitboxes.hpp"
#include "../combat/PlantedBomb.hpp"
#include "../combat/BombField.hpp"
#include "../economy/BuyInventory.hpp"
#include "../economy/BuyMenu.hpp"
#include "../combat/WeaponRecoil.hpp"
#include "../combat/MeleeAttack.hpp"
#include "../input/MouseLook.hpp"
#include "../input/WeaponWheel.hpp"
#include "../input/CameraWheel.hpp"
#include "../config/GameplaySettings.hpp"
#include "../inventory/AmmoRecovery.hpp"
#include "../inventory/ItemPickup.hpp"
#include "../squad/SquadController.hpp"
#include "../squad/ArmyAI.hpp"
#include "../platform/SpellCaster.hpp"
#include "../platform/NativeUnitHud.hpp"
#include "InventoryController.hpp"
#include "../input/SquadOrderMode.hpp"
#include "../presentation/ControlView.hpp"
#include <string>
#include <vector>
#include <chrono>

class ShooterController {
public:
    void Tick(uintptr_t ui);
    void TickArmyAI(); // Construction may instantiate models: run only after the completed native frame.
    void ResetMap();
    // A caught extension fault must return camera/HUD ownership so native menus remain usable.
    void RecoverFault() { Disable(true); }
    // Native window input feeds the dedicated relative-look collector on the same game thread.
    void AttachWindow(HWND window) { mouseLook_.Attach(window); }
    void MouseInput(LPARAM packet,bool accept) { mouseLook_.Input(packet,accept); }
    void ResetMouse() { mouseLook_.Reset(); weaponWheel_.Reset();cameraWheel_.Reset();squadOrderMode_.ClearInput();armyToggleRequested_=false; }
    // Zoom is presentation-only: it must not cancel channeling or switch a carried CS weapon in TPS.
    void RequestMouseWheel(int delta) {
        if(!Visible() || Buying() || InventoryOpen() || UnitFirstPerson())return;
        if(ThirdPerson())cameraWheel_.Add(delta);
        else {RequestSpellCancel();weaponWheel_.Add(delta);}
    }
    float ThirdPersonDistance() const {return thirdPersonDistance_;}
    void RequestToggle() { toggleRequested_ = true; }
    // T switches presentation/combat while preserving the same controlled unit and spell system.
    void RequestViewToggle() {viewToggleRequested_=true;}
    void RequestThirdPersonToggle() {thirdPersonRequested_=true;}
    bool ThirdPerson() const {return view_==ControlView::ThirdPerson;}
    bool UnitFirstPerson() const {return view_==ControlView::UnitFirstPerson;}
    // The native guard queries live mode/config instead of modifying saved vulnerability or alliance flags.
    wc3::Handle EnemyProtectedActor() const {return Visible() && UnitFirstPerson() && settings_.unitViewPreventEnemyAttacks ? unit_ : 0;}
    void RequestUnitViewToggle() {unitViewRequested_=true;}
    // Rendering must consume the repaired attack pose after the game-thread visual clock advances.
    bool ManualAttackAnimating() const {return Visible() && ThirdPerson() && heroAttack_.Animating(GetTickCount()*.001);}
    const UnitAimTarget& AimTarget() const {return aimTarget_;}
    void RequestMenu() { menuRequested_ = true; }
    void RequestRefill() { refillRequested_ = true; }
    // All-weapons now uses F12; F9 owns the independent unarmed unit view.
    void RequestAllWeapons() { allWeaponsRequested_=true; }
    void RequestItemPickup() { itemRequested_=true; }
    bool Shopping() const {return shop_!=0;}
    void RequestShopClose() {shopCloseRequested_=true;}
    void RequestSquad(bool passive) { squadRequested_=passive ? 2 : 1; }
    // Release shares the command mailbox: the last H/O/J tap before a frame wins.
    void RequestSquadRelease() { squadRequested_=3; }
    void RequestSquadFocus() {squadRequested_=4;}
    void RequestSquadOrdersToggle() {if(!ThirdPerson() && !UnitFirstPerson())squadOrderMode_.QueueToggle();}
    void RequestArmyAI() {armyToggleRequested_=!armyToggleRequested_;}
    bool ArmyAIActive() const {return armyAI_.Active();}
    bool SquadOrdersInFPS() const {return squadOrderMode_.RoutingEnabled();}
    bool SquadReticleOrdersEnabled() const {
        return Visible() && settings_.squadReticleAttack && (ThirdPerson() || UnitFirstPerson() || SquadOrdersInFPS());
    }
    bool BlocksPassiveUnit(uintptr_t object) const { return (Visible() || Shopping()) && squad_.BlocksUnit(object); }
    bool BlocksPassiveAttack(uintptr_t attack) const { return (Visible() || Shopping()) && squad_.BlocksAttack(attack); }
    size_t SquadCount() const { return squad_.Count(); }
    bool SquadPassive() const { return squad_.Passive(); }
    // Explicit spell approach/channeling may use the native motor; idle FPS never does.
    bool NativeCastMovement() const { return Visible() && Casting(); }
    void RequestSettingsReload() { settingsRequested_=true; }
    // Queue the display switch for the simulation thread, like the other FPS controls.
    void RequestSpeedToggle() { speedToggleRequested_=true; }
    // Fog changes follow the same input-to-simulation mailbox as the speed display.
    void RequestFogToggle() { fogToggleRequested_=true; }
    // Queue one switch for both teams; save preferences only on the game thread.
    void RequestHealthBarToggle() { healthBarToggleRequested_=true; }
    // Queue allied targeting changes; Windows key repeat must not flip the mode every frame.
    void RequestAllyTargetToggle() {allyTargetToggleRequested_=true;}
    // Spell and interruption taps are executed on the game thread, never inside WindowProc.
    void RequestSpell(int slot) { spellRequested_=slot; }
    void RequestSpellCancel() { spellCancelRequested_=true; }
    std::string Shortcut(ShortcutAction action) const {return ShortcutBindings::Label(settings_.shortcuts.Key(action));}
    bool SpeedVisible() const { return showSpeed_; }
    // Input queues the toggle; only the simulation tick changes the HUD preference.
    void RequestMinimapToggle() { minimapToggleRequested_=true; }
    bool MinimapVisible() const { return !UnitFirstPerson() && showMinimap_; }
    const SpellHudSlots& SpellHud() const {return spellHud_;}
    const UnitHudSnapshot& UnitHud() const {return unitHud_;}
    const InventoryControls& ItemControls() const {return itemInventory_.State();}
    bool InventoryOpen() const {return itemInventory_.State().Open();}
    bool InventoryInputOpen() const {return itemInventory_.State().RoutingOpen();}
    bool CursorMode() const {return Buying() || InventoryOpen();}
    void RequestInventory(InventoryControls::Event e) {itemInventory_.Input(e);}
    void RequestInventoryClose() {inventoryEscape_=true;itemInventory_.Input({InventoryControls::Kind::Close});}
    void InventoryProjection(float verticalFov) {inventoryVerticalFov_=verticalFov;}
    wc3::Handle HudActor() const {return Visible() && !UnitFirstPerson() ? unit_ : 0;}
    float DisplaySpeed() const { return movement_.Speed()/MovementPhysics::worldScale; }
    // Buy-menu events are consumed on Warcraft's simulation thread, never inside WindowProc.
    void RequestBuyToggle() { buyToggleRequested_=true; }
    void RequestBuyAmmo() { buyAmmoRequested_=true; }
    void RequestBuyKey(int key) { buyKeyRequested_=key; }
    void RequestBuyClick(int x,int y,int width,int height);
    bool Buying() const { return buyMenu_.Open(); }
    BuyMenu::Page BuyPage() const { return buyMenu_.Current(); }
    const GameplaySettings& Settings() const { return settings_; }
    bool OwnsWeapon(int slot) const { return slot>=0 && slot<WeaponSlots::Count && inventory_.owned[slot]; }
    int Gold() const { return gold_; }
    float ViewYaw() const { return yaw_+(ThirdPerson() ? 0 : recoil_.Yaw()); }
    float ViewPitch() const { return pitch_+(ThirdPerson() ? 0 : recoil_.Pitch()); }
    float ViewFov() const { return scopeLevel_==2 ? 10.0f : Scoped() ? 40.0f : 85.0f; }
    const std::string& SkyName() const {
        const auto& choice=settings_.tilesetSky[static_cast<unsigned char>(mapTileset_)];
        return choice.empty() ? settings_.defaultSky : choice;
    }
    bool Visible() const { return active_ && !suspended_ && !Shopping(); }
    // Presentation may identify the controlled actor only while the first-person view owns the screen.
    wc3::Handle ViewActor() const { return Visible() ? unit_ : 0; }
    // Actor visibility is separate from simulation ownership: TPS retains guards but shows the native body.
    wc3::Handle HiddenActor() const {return ThirdPerson() ? 0 : ViewActor();}
    // Physical feet are a presentation snapshot; the render adapter never changes native fly height.
    float ActorFeet() const {return movement_.FeetZ();}
    wc3::Handle JumpViewActor() const {return ThirdPerson() && !status_.contained && !status_.hidden ? ViewActor() : 0;}
    // Block only map camera writes during live FPS; our camera pass and cinematic/RTS writes remain allowed.
    bool BlocksMapCamera() const { return Visible() && !ownCameraRequest_; }
    void SetPaused(bool value) { paused_ = value; }
    int WeaponIndex() const { return weapon_; }
    const char* Animation() const;
    float AnimationTime() const;
    const ViewmodelPose& WeaponPose() const {return viewmodelMotion_.Pose();}
    unsigned ShotSerial() const {return shotSerial_;}
    bool ShowMuzzleFlash() const {
        return flashWeapon_==weapon_ && WeaponSlots::Firearm(weapon_) && settings_.viewmodel.muzzleFlash &&
            Visible() && !ThirdPerson() && !UnitFirstPerson() && !settings_.warcraftOnly && !Buying() && !InventoryOpen();
    }
    float Health() const { return health_; }
    float Recoil() const { return recoil_.Magnitude(); }
    bool Scoped() const { return scopeLevel_ != 0; }
    const UnitStatus& Status() const { return status_; }
#if defined(WCS_STATUS_TEST) || defined(WCS_GAMEPLAY_TEST) || defined(WCS_TREE_WHEEL_TEST)
    // Expose the active unit only to the separate, opt-in native spell test build.
    wc3::Handle TestUnit() const { return Visible() ? unit_ : 0; }
#endif
#ifdef WCS_GAMEPLAY_TEST
    // Aim the actual FPS/TPS camera ray at a disposable submerged target, including boom parallax.
    void TestAimPoint(float x,float y,float z) {
        recoil_.Reset();
        for(int pass=0;pass<6;++pass) {
            float origin[3],direction[3];AimRay(origin,direction);
            float dx=x-origin[0],dy=y-origin[1];
            yaw_=std::atan2(dy,dx)/.01745329252f;pitch_=std::atan2(z-origin[2],std::hypot(dx,dy))/.01745329252f;
        }
    }
    // Native pose/cooldown samples are confined to a disposable animation regression build.
    void TestAnimationState() const {wc3::Log("fixture pose busy=%d animating=%d sequence=%d rate=%.3f speed=%.1f",heroAttack_.Busy(GetTickCount()*.001),heroAttack_.Animating(GetTickCount()*.001),wc3::AnimationSequence(unit_),wc3::AnimationRate(unit_),movement_.Speed());}
    // Opt-in native oracle uses the production windup/contact, without injecting desktop mouse input.
    void TestHeroAttack() {if(ThirdPerson()){unitLocomotion_.Release();heroAttack_.Begin(unit_,GetTickCount()*.001,settings_,status_);}}
    // Held attack exercises the normal production branch and autoattack guard, rather than starting the timeline directly.
    void TestHoldAttack(DWORD duration) {testAttackUntil_=GetTickCount()+std::min(duration,DWORD(10000));}
    void TestLogTarget() const {wc3::Log("fixture target hp=%.1f actorMana=%.1f",fixtureTarget_ ? wc3::Real(wc3::GetUnitState(fixtureTarget_,0)) : -1,wc3::Real(wc3::GetUnitState(unit_,2)));}
    // Match the real movement key-down path, including deliberate channel interruption.
    void TestWalk(int key,DWORD duration,bool walking=false) {RequestSpellCancel();testWalkKey_=key;testWalking_=walking;testWalkUntil_=GetTickCount()+std::min(duration,DWORD(2000));}
    // Disposable scenes exercise the real weapon contact path without desktop mouse input.
    void TestStrike(int slot) {
        if(slot<0 || slot>=WeaponSlots::Count || slot==WeaponSlots::C4)return;
        weapon_=slot;Strike();
        // Observe real health/mana/stun after production impact; no synthetic passive effects in the oracle.
        wc3::Log("fixture contact actorHP=%.2f targetMana=%.2f targetStuns=%d targetBash=%d actorOrder=%d",
            wc3::Real(wc3::GetUnitState(unit_,0)),fixtureTarget_ ? wc3::Real(wc3::GetUnitState(fixtureTarget_,2)) : -1,
            fixtureTarget_ ? UnitStatus::Read(fixtureTarget_,gameBase_).stunCount : -1,
            fixtureTarget_ && wc3::HasUnitBuff(fixtureTarget_,0x42505345),wc3::GetUnitCurrentOrder(unit_));
    }
    // Temporary native fixtures inspect/reduce ammunition without spawning anything in ordinary builds.
    void TestDrainAmmo() { for (int i=0;i<WeaponSlots::Count;++i) ammo_[i]=reserve_[i]=0;ammoRecovery_.Reset(); }
    void TestLogSquad() const { squad_.TestLog(); }
    void TestLogAmmo() const { wc3::Log("fixture ammo AK=%d/%d M4=%d/%d USP=%d/%d AWP=%d/%d C4=%d",
        ammo_[0],reserve_[0],ammo_[1],reserve_[1],ammo_[2],reserve_[2],ammo_[3],reserve_[3],ammo_[5]); }
#endif
    void InterfaceRequest(bool show);
    float MovementSpeed() const { return movement_.Speed(); }
    float Stance() const { return movement_.Duck(); }
    int Ammo() const { return ammo_[weapon_]; }
    int Reserve() const { return reserve_[weapon_]; }
    bool RefillNotice() const { return refillTick_ && GetTickCount() - refillTick_ < 1600; }
    const char* AmmoMessage() const { return ammoMessage_.c_str(); }
    bool ItemNearby() const { return itemNearby_; }
    float BombRemaining() const { return bombs_.Remaining(); }
    size_t BombCount() const { return bombs_.Count(); }
    float PlantProgress() const { return plantProgress_; }
    const HitFeedback& Hits() const { return hits_; }
    void Configure(const char* root, uintptr_t gameBase);
    static const Weapon* const weapons; // Compatibility defaults for private fixtures.
    const Weapon& WeaponData(int slot) const {return settings_.weapons[slot];}
private:
#ifdef WCS_GAMEPLAY_TEST
    int testWalkKey_=0;bool testWalking_=false;DWORD testWalkUntil_=0,testAttackUntil_=0;
#endif
    bool Pressed(int key);
    bool Down(int key) const;
    bool Down(ShortcutAction action) const {return Down(settings_.shortcuts.Key(action));}
    bool Pressed(ShortcutAction action) {return Pressed(settings_.shortcuts.Key(action));}
    void Toggle(ControlView initial=ControlView::WeaponsFirstPerson);
    void ToggleView();
    void SelectView(ControlView next);
    void UpdateSquad(DWORD now);
    void AimRay(float* origin,float* direction);
    SpellAim InventoryAim(int x,int y,int width,int height);
    bool Casting() const {return spells_.Active() || itemInventory_.Active();}
    void CancelCasting() {spells_.Cancel();itemInventory_.CancelCasting();}
    void Disable(bool restoreCamera);
    void AimAndMove(float dt);
    bool Fire(bool secondary = false,bool burstRound=false,bool interruptReload=false);
    void FireGlock(bool primaryPressed,bool secondaryPressed,DWORD now);
    BurstFire glockBurst_;
    void Strike(float yawOffset=0,float pitchOffset=0,float damageScale=1);
    void Reload();
    ShotgunReload shotgunReload_;
    DWORD shotgunReloadStart_=0;
    int shotgunReloadPhase_=0;
    bool shotgunReloadPerShell_=true,shotgunReloadInterrupt_=true;
    void SwitchWeapon(int slot);
    void PlantC4(float dt);
    void CancelPlant();
    void RefillAmmo(bool grantAll=false);
    void PickupItem();
    bool OpenShop();
    void CloseShop();
    wc3::Handle shop_=0;
    bool shopCloseRequested_=false;
    void ReloadSettings();
    void ToggleHealthBars();
    void ResetLoadout();
    void Buy(int slot,bool ammunition);
    void UpdateBuyMenu();
    void Notice(const char* message);
    float Play(const char* name,float rate=1,float elapsed=0);
    void Camera();
    ThirdPersonSettings CameraSettings() const;
    void Sound(const char* name);
    void SurfaceStep(float volume);
    void UpdateStatus();
    const char* Sequence(const char* name) const;
    wc3::Handle unit_ = 0;
    wc3::Handle fixtureTarget_ = 0; // only used after an explicit private verification request
    wc3::Handle fixtureGate_ = 0; // destructables require their own removal native
    std::vector<wc3::Handle> blastFixtures_; // explicit private verification only
    uintptr_t ui_ = 0;
    uintptr_t gameBase_ = 0;
    UnitStatus status_;
    bool active_ = false, suspended_ = false, paused_ = false;
    bool cinematicRequested_ = false;
    bool ownCameraRequest_ = false;
    int scopeLevel_ = 0;
    bool toggleRequested_ = false, menuRequested_ = false, refillRequested_ = false, allWeaponsRequested_=false;
    bool itemRequested_=false,settingsRequested_=false,itemNearby_=false;
    bool speedToggleRequested_=false,showSpeed_=false;
    bool minimapToggleRequested_=false,showMinimap_=true;
    bool fogToggleRequested_=false,fogOfWarDisabled_=false;
    bool healthBarToggleRequested_=false;
    bool allyTargetToggleRequested_=false;
    int spellRequested_=-1;
    bool spellCancelRequested_=false;
    SpellCaster spells_;
    InventoryController itemInventory_;
    bool inventoryEscape_=false;
    float inventoryVerticalFov_=85;
    SpellHudSlots spellHud_={};
    DWORD spellHudTick_=0,spellHudLog_=0;
    int squadRequested_=0;
    bool keys_[256] = {};
    int weapon_ = 0, ammo_[WeaponSlots::Count] = {}, reserve_[WeaponSlots::Count] = {};
    float yaw_ = 0, pitch_ = 0, health_ = 0;
    // Movement needs a finer clock than GetTickCount's ~16ms steps once rendering exceeds 64 FPS.
    std::chrono::steady_clock::time_point tick_{};
    DWORD lastShot_ = 0, reloadEnd_ = 0, animationTick_ = 0;
    ViewmodelMotion viewmodelMotion_;
    unsigned shotSerial_=0;
    int flashWeapon_=-1;
    std::string root_, animation_ = "idle";
    MeleeAttack meleeAttack_ = MeleeAttack::Select(WeaponSlots::Knife, false);
    WeaponRecoil recoil_;
    DWORD meleeContact_ = 0, meleeReady_ = 0; // contact and recovery follow the visible swing
    MovementPhysics movement_;
    WarcraftCollision collision_;
    FirstPersonCamera camera_;
    ThirdPersonCamera thirdCamera_;
    UnitAttack heroAttack_;
    UnitLocomotion unitLocomotion_;
    UnitAimTarget aimTarget_;
    ControlView view_=ControlView::WeaponsFirstPerson;
    bool viewToggleRequested_=false,thirdPersonRequested_=false,unitViewRequested_=false;
    NativeSky nativeSky_;
    NativeFogOfWar nativeFog_;
    MouseLook mouseLook_;
    WeaponWheel weaponWheel_;
    CameraWheel cameraWheel_;
    float thirdPersonDistance_=500;
    GameplaySettings settings_;
    NativeUnitHud unitHudReader_;
    UnitHudSnapshot unitHud_;
    DWORD unitHudTick_=0;
    BuyInventory inventory_;
    BuyMenu buyMenu_;
    bool buyToggleRequested_=false,buyAmmoRequested_=false,suppressFire_=false;
    int buyKeyRequested_=-1,gold_=0;
    char mapTileset_=0;
    AmmoRecovery ammoRecovery_;
    ItemPickup itemPickup_;
    SquadController squad_;
    ArmyAI armyAI_;
    bool armyToggleRequested_=false;
    SquadOrderMode squadOrderMode_;
    std::string ammoMessage_;
    GameAudio audio_;
    float stepDistance_ = 0;
    unsigned stepIndex_ = 0;
    unsigned knifeSwing_ = 0;
    float animationRate_ = 1;
    DWORD refillTick_ = 0;
    HitFeedback hits_;
    UnitHitboxes hitboxes_;
    DestructableHitboxes destructableHitboxes_;
    BombField bombs_;
    float plantProgress_ = 0, plantX_ = 0, plantY_ = 0;
};
