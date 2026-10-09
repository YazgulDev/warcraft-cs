#pragma once

struct MoveInput {
    float forward = 0, right = 0, yaw = 0;
    bool walk = false, duck = false, jump = false;
    // Spell roots/stuns stop horizontal momentum and new jumps while gravity continues.
    bool immobilized = false;
    bool stanceLocked = false;
};

class MovementPhysics {
public:
    // One GoldSrc unit is scaled to 1.5 Warcraft units for this world's proportions.
    static constexpr float worldScale = 1.5f;
    void Reset(float floor);
    void Step(const MoveInput& input, float dt, float weaponSpeed);
    void ResolveFloor(float floor);
    void BlockX() { vx_ = 0; }
    void BlockY() { vy_ = 0; }
    void Stop() { vx_ = vy_ = 0; }
    void LimitSpeed(float maximum);
    float VelocityX() const { return vx_; }
    float VelocityY() const { return vy_; }
    float Speed() const;
    float FeetZ() const { return feetZ_; }
    float EyeZ() const { return feetZ_ + (64 - 28 * duck_) * worldScale; }
    float Duck() const { return duck_; }
    bool Grounded() const { return grounded_; }
private:
    float vx_ = 0, vy_ = 0, vz_ = 0, feetZ_ = 0, duck_ = 0;
    bool grounded_ = true, jumpHeld_ = false;
};
