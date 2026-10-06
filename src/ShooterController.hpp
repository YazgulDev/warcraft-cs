#pragma once
#include "WeaponSlots.hpp"
#include "WarcraftApi.hpp"
#include "MovementPhysics.hpp"
#include "WarcraftCollision.hpp"
#include "FirstPersonCamera.hpp"
#include "GameAudio.hpp"
#include "FullscreenView.hpp"
#include "HitFeedback.hpp"
#include "UnitStatus.hpp"
#include "UnitHitboxes.hpp"
#include "DestructableHitboxes.hpp"
#include "PlantedBomb.hpp"
#include "WeaponRecoil.hpp"
#include "MeleeAttack.hpp"
#include "MouseLook.hpp"
#include "WeaponWheel.hpp"
#include "GameplaySettings.hpp"
#include "AmmoRecovery.hpp"
#include "ItemPickup.hpp"
#include "SquadController.hpp"
#include <string>
#include <vector>

struct Weapon {
    const char* name;
    const char* sound;
    int magazine;
    int reserve;
    float damage;
    float interval;
    float reload;
    float range;
    bool automatic;
};

class ShooterController {
public:
    void Tick(uintptr_t ui);
    void ResetMap();
    // A caught extension fault must return camera/HUD ownership so native menus remain usable.
    void RecoverFault() { Disable(true); }
    // Native window input feeds the dedicated relative-look collector on the same game thread.
    void AttachWindow(HWND window) { mouseLook_.Attach(window); }
    void MouseInput(LPARAM packet,bool accept) { mouseLook_.Input(packet,accept); }
    void ResetMouse() { mouseLook_.Reset(); weaponWheel_.Reset(); }
    void RequestWeaponWheel(int delta) { weaponWheel_.Add(delta); }
    void RequestToggle() { toggleRequested_ = true; }
    void RequestMenu() { menuRequested_ = true; }
    void RequestRefill() { refillRequested_ = true; }
    void RequestItemPickup() { itemRequested_=true; }
    void RequestSquad(bool passive) { squadRequested_=passive ? 2 : 1; }
    // Release shares the command mailbox: the last H/O/J tap before a frame wins.
    void RequestSquadRelease() { squadRequested_=3; }
    bool BlocksPassiveUnit(uintptr_t object) const { return Visible() && squad_.BlocksUnit(object); }
    bool BlocksPassiveAttack(uintptr_t attack) const { return Visible() && squad_.BlocksAttack(attack); }
    size_t SquadCount() const { return squad_.Count(); }
    bool SquadPassive() const { return squad_.Passive(); }
    void RequestSettingsReload() { settingsRequested_=true; }
    bool Visible() const { return active_ && !suspended_; }
    // Presentation may identify the controlled actor only while the first-person view owns the screen.
    wc3::Handle ViewActor() const { return Visible() ? unit_ : 0; }
    // Block only map camera writes during live FPS; our camera pass and cinematic/RTS writes remain allowed.
    bool BlocksMapCamera() const { return Visible() && !ownCameraRequest_; }
    void SetPaused(bool value) { paused_ = value; }
    int WeaponIndex() const { return weapon_; }
    const char* Animation() const;
    float AnimationTime() const;
    float Health() const { return health_; }
    float Recoil() const { return recoil_.Magnitude(); }
    bool Scoped() const { return scopeLevel_ != 0; }
    const UnitStatus& Status() const { return status_; }
#if defined(WCS_STATUS_TEST) || defined(WCS_GAMEPLAY_TEST) || defined(WCS_TREE_WHEEL_TEST)
    // Expose the active unit only to the separate, opt-in native spell test build.
    wc3::Handle TestUnit() const { return Visible() ? unit_ : 0; }
#endif
#ifdef WCS_GAMEPLAY_TEST
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
    float BombRemaining() const { return bomb_.Active() ? bomb_.Remaining() : -1; }
    float PlantProgress() const { return plantProgress_; }
    const HitFeedback& Hits() const { return hits_; }
    void Configure(const char* root, uintptr_t gameBase);
    static const Weapon weapons[WeaponSlots::Count];
private:
    bool Pressed(int key);
    bool Down(int key) const;
    void Toggle();
    void Disable(bool restoreCamera);
    void AimAndMove(float dt);
    void Fire(bool secondary = false);
    void Strike();
    void Reload();
    void SwitchWeapon(int slot);
    void PlantC4(float dt);
    void CancelPlant();
    void RefillAmmo();
    void PickupItem();
    void ReloadSettings();
    float Play(const char* name);
    void Camera();
    void Sound(const char* name);
    void SurfaceStep(float volume);
    void SetInterface(bool show);
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
    bool ownInterfaceRequest_ = false, cinematicRequested_ = false;
    bool ownCameraRequest_ = false;
    int scopeLevel_ = 0;
    bool toggleRequested_ = false, menuRequested_ = false, refillRequested_ = false;
    bool itemRequested_=false,settingsRequested_=false,itemNearby_=false;
    int squadRequested_=0;
    bool keys_[256] = {};
    int weapon_ = 0, ammo_[WeaponSlots::Count] = {}, reserve_[WeaponSlots::Count] = {};
    float yaw_ = 0, pitch_ = 0, health_ = 0;
    DWORD tick_ = 0, lastShot_ = 0, reloadEnd_ = 0, animationTick_ = 0;
    std::string root_, animation_ = "idle";
    MeleeAttack meleeAttack_ = MeleeAttack::Select(WeaponSlots::Knife, false);
    WeaponRecoil recoil_;
    DWORD meleeContact_ = 0, meleeReady_ = 0; // contact and recovery follow the visible swing
    MovementPhysics movement_;
    WarcraftCollision collision_;
    FirstPersonCamera camera_;
    MouseLook mouseLook_;
    WeaponWheel weaponWheel_;
    GameplaySettings settings_;
    AmmoRecovery ammoRecovery_;
    ItemPickup itemPickup_;
    SquadController squad_;
    std::string ammoMessage_;
    GameAudio audio_;
    FullscreenView fullscreen_;
    float stepDistance_ = 0;
    unsigned stepIndex_ = 0;
    unsigned knifeSwing_ = 0;
    float animationRate_ = 1;
    DWORD refillTick_ = 0;
    HitFeedback hits_;
    UnitHitboxes hitboxes_;
    DestructableHitboxes destructableHitboxes_;
    PlantedBomb bomb_;
    float plantProgress_ = 0, plantX_ = 0, plantY_ = 0;
};
