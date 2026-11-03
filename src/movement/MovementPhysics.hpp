#pragma once
#include "MovementSettings.hpp"
#include "MovementSurface.hpp"
#include "WaterContact.hpp"

struct MoveInput {
    float forward = 0, right = 0, yaw = 0;
    bool walk = false, duck = false, jump = false;
    // Spell roots/stuns stop horizontal momentum and new jumps while gravity continues.
    bool immobilized = false;
    bool stanceLocked = false;
    bool waterExit = false; // Native shore probe permits an exit; the usual body sweeps still own clearance.
};

class MovementPhysics {
public:
    // A native cliff face supplies no support even when its visual height is interpolated smoothly.
    void BeginFall() { if (grounded_) { grounded_ = false; vz_ = 0; } }
    // One GoldSrc unit is scaled to 1.5 Warcraft units for this world's proportions.
    static constexpr float worldScale = 1.5f;
    void Reset(float floor);
    void Configure(const MovementSettings& settings) { settings_ = settings; }
    void Step(const MoveInput& input, float dt, float weaponSpeed);
    void SetWater(const WaterContact& water);
    bool Swimming() const {return swimming_;}
    bool SwimmingEnabled() const {return settings_.swimming.enabled && waterAllowed_;}
    void AllowSwimming(bool allowed) {waterAllowed_=allowed;if(!allowed)SetWater({});}
    bool Wet() const {return water_.present && feetZ_<=water_.surface;}
    float WaterLevel() const {return water_.surface;}
    float WaterExitHeight() const {return settings_.swimming.exitHeight;}
    float WaterExitReach() const {return settings_.swimming.exitReach;}
    void ResolveFloor(float floor);
    void ResolveClassicFloor(float floor);
    bool ResolveClassicRamp(const MovementSurface& surface);
    bool ResolveSurf(const MovementSurface& surface);
    void FinishMove(); // Confirm an actual takeoff only after collision has accepted the frame.
    bool Surfing() const {return surfing_;}
    bool SurfEnabled() const {return settings_.surf.enabled;}
    float SurfMaximumAngle() const {return settings_.surf.maximumAngle;}
    float SurfContactDistance() const {return settings_.surf.contactDistance;}
    void ResolveSurface(const MovementSurface& surface);
    void ClipHead(float ceiling);
    void SlideHorizontal(float nx,float ny);
    void BlockX() { vx_ = 0; }
    void BlockY() { vy_ = 0; }
    void Stop() { vx_ = vy_ = 0; }
    void LimitSpeed(float maximum);
    float VelocityX() const { return vx_; }
    float VelocityY() const { return vy_; }
    float Speed() const;
    float FeetZ() const { return feetZ_; }
    float PreviousFeetZ() const { return previousFeetZ_; }
    float VerticalVelocity() const { return vz_; }
    float StepHeight() const { return settings_.stepHeight; }
    float MaxWalkSlope() const { return settings_.maxWalkSlope; }
    float HullRadius() const { return settings_.hullRadius; }
    bool ObjectMeshCollision() const { return settings_.objectMeshCollision; }
    bool PlatformRecovery() const {return settings_.platformRecovery;}
    bool ClassicCollision() const {return settings_.classicCollision;}
    bool ClassicRampContacts() const {return settings_.classicRampContacts;}
    bool RecoverInvalidPosition() const {return settings_.recoverInvalidPosition;}
    float RecoveryDistance() const {return settings_.recoveryDistance;}
    // Emergency placement discards fall velocity without changing stance, tuning or held-jump state.
    void RecoverFloor(float floor) {feetZ_=previousFeetZ_=floor;vx_=vy_=vz_=0;grounded_=true;}
    float BodyHeight() const { return (64-28*duck_)*worldScale+12; }
    bool NativeUnitCollision() const { return settings_.useNativeUnitCollision; }
    float EyeZ() const { return feetZ_ + (64 - 28 * duck_) * worldScale; }
    float Duck() const { return duck_; }
    bool Grounded() const { return grounded_; }
private:
    void Swim(const MoveInput& input,float dt,float weaponSpeed);
    float FloatFeet() const;
    WaterContact water_;
    bool swimming_=false,waterAllowed_=true,waterJump_=false;
    float vx_ = 0, vy_ = 0, vz_ = 0, feetZ_ = 0, duck_ = 0;
    float previousFeetZ_ = 0;
    float previousDuck_ = 0;
    MovementSettings settings_;
    bool grounded_ = true, jumpHeld_ = false, jumpSuppressed_ = false;
    bool surfing_=false;
    float pendingJumpBoost_=0;
};
