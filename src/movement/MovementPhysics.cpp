#include "MovementPhysics.hpp"
#include <algorithm>
#include <cmath>

void MovementPhysics::Reset(float floor) {
    vx_ = vy_ = vz_ = duck_ = 0; feetZ_ = floor;
    previousFeetZ_ = floor;
    grounded_ = true; jumpHeld_ = jumpSuppressed_ = false;
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
    previousFeetZ_ = feetZ_;
    // Smooth stance changes avoid teleporting the camera when Ctrl is pressed.
    float targetDuck = input.duck ? 1.0f : 0.0f;
    // Incapacitation freezes even an in-progress crouch; roots alone still permit stance changes.
    if (!input.stanceLocked) duck_ += std::clamp(targetDuck - duck_, -dt / 0.2f, dt / 0.2f);
    if (input.immobilized) Stop();
    // Space held through a root/stun must be released before autojump may resume.
    if (!input.jump) jumpSuppressed_ = false;
    else if (input.immobilized) jumpSuppressed_ = true;
    // Autojump repeats only on valid ground contact; manual mode retains Space's rising edge.
    bool jump = input.jump && !jumpSuppressed_ && (!jumpHeld_ || settings_.autoJump) && grounded_ && !input.immobilized;
    jumpHeld_ = input.jump;
    if (grounded_ && !jump) {
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
        float acceleration = grounded_ ? 10.0f : settings_.airAcceleration;
        float addition = std::max(0.0f, std::min(needed, acceleration * speed * dt));
        vx_ += addition * wx; vy_ += addition * wy;
    }
    if (jump) {
        // A timed hop skips ground friction and adds bounded momentum, while Shift/Ctrl retain slow movement.
        if (settings_.bunnyHop && !input.walk && !input.duck && length > 0) {
            float speed = Speed();
            float boosted = std::min(speed * (1 + settings_.jumpBoostPercent / 100), settings_.maxBunnySpeed * worldScale);
            if (speed > 0 && boosted > speed) { vx_ *= boosted / speed; vy_ *= boosted / speed; }
        }
        vz_ = settings_.jumpSpeed * worldScale; grounded_ = false;
    }
    if (settings_.bunnyHop) LimitSpeed(settings_.maxBunnySpeed * worldScale);
    if (!grounded_) {
        // Integrate gravity around the position step so jump height varies little with frame rate.
        float gravity = settings_.gravity * worldScale;
        feetZ_ += vz_ * dt - 0.5f * gravity * dt * dt;
        vz_ -= gravity * dt;
    }
}
void MovementPhysics::ResolveFloor(float floor) {
    // Losing support starts a fall at the current height instead of snapping down the cliff.
    if (grounded_ && feetZ_ - floor > settings_.stepHeight) { grounded_ = false; vz_ = 0; return; }
    // Rising players cannot be teleported onto overhead geometry; collision rejects its sides.
    if (grounded_ || (vz_ <= 0 && feetZ_ <= floor)) { feetZ_ = floor; vz_ = 0; grounded_ = true; }
}
