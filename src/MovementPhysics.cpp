#include "MovementPhysics.hpp"
#include <algorithm>
#include <cmath>

void MovementPhysics::Reset(float floor) {
    vx_ = vy_ = vz_ = duck_ = 0; feetZ_ = floor;
    grounded_ = true; jumpHeld_ = false;
}
float MovementPhysics::Speed() const { return std::hypot(vx_, vy_); }
void MovementPhysics::LimitSpeed(float maximum) {
    // Applying a slow must also constrain existing momentum, including airborne strafing.
    float speed = Speed(); maximum = std::max(0.0f, maximum);
    if (speed > maximum) { vx_ *= maximum / speed; vy_ *= maximum / speed; }
}
void MovementPhysics::Step(const MoveInput& input, float dt, float weaponSpeed) {
    dt = std::clamp(dt, 0.0f, 0.05f);
    if (!dt) return;
    // Smooth stance changes avoid teleporting the camera when Ctrl is pressed.
    float targetDuck = input.duck ? 1.0f : 0.0f;
    // Incapacitation freezes even an in-progress crouch; roots alone still permit stance changes.
    if (!input.stanceLocked) duck_ += std::clamp(targetDuck - duck_, -dt / 0.2f, dt / 0.2f);
    if (input.immobilized) Stop();
    bool jump = input.jump && !jumpHeld_ && grounded_ && !input.immobilized;
    jumpHeld_ = input.jump;
    if (jump) { vz_ = 268.328f * worldScale; grounded_ = false; }
    if (grounded_) {
        // Friction persists without movement keys, producing CS-style stopping and counter-strafing.
        float speed = Speed();
        if (speed > 0) {
            float remaining = std::max(0.0f, speed - std::max(speed, 75 * worldScale) * 4 * dt);
            vx_ *= remaining / speed; vy_ *= remaining / speed;
        }
    }
    float length = input.immobilized ? 0 : std::hypot(input.forward, input.right);
    if (length > 0) {
        float angle = input.yaw * 0.01745329252f;
        float wx = (input.forward * std::cos(angle) + input.right * std::sin(angle)) / length;
        float wy = (input.forward * std::sin(angle) - input.right * std::cos(angle)) / length;
        float speed = weaponSpeed * worldScale;
        if (input.walk) speed *= 0.52f;
        if (input.duck) speed *= 0.34f;
        // Air acceleration is limited along the wish direction, preserving air strafing inertia.
        float cap = grounded_ ? speed : std::min(speed, 30 * worldScale);
        float needed = cap - (vx_ * wx + vy_ * wy);
        float addition = std::max(0.0f, std::min(needed, 10 * speed * dt));
        vx_ += addition * wx; vy_ += addition * wy;
    }
    if (!grounded_) {
        // Integrate gravity around the position step so jump height varies little with frame rate.
        float gravity = 800 * worldScale;
        feetZ_ += vz_ * dt - 0.5f * gravity * dt * dt;
        vz_ -= gravity * dt;
    }
}
void MovementPhysics::ResolveFloor(float floor) {
    // The feet never penetrate the sampled terrain, including on landing and uphill steps.
    if (grounded_ || feetZ_ <= floor) { feetZ_ = floor; vz_ = 0; grounded_ = true; }
}
