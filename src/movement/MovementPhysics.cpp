#include "MovementPhysics.hpp"
#include <algorithm>
#include <cmath>

void MovementPhysics::Reset(float floor) {
    vx_ = vy_ = vz_ = duck_ = 0; feetZ_ = floor;
    previousFeetZ_ = floor; previousDuck_=0;
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
    previousDuck_=duck_;
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
    if (jump) {
        // Takeoff uses air acceleration immediately: landing must not inject a ground-speed strafe impulse.
        if (!settings_.bunnyHop) LimitSpeed(weaponSpeed * worldScale);
        vz_ = settings_.jumpSpeed * worldScale; grounded_ = false;
    }
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
        // Cap only the velocity projected onto the input direction. Turning A/D adds sideways
        // momentum above running speed; holding forward alone cannot keep accelerating.
        float cap = grounded_ ? speed : std::min(speed, 30 * worldScale);
        float needed = cap - (vx_ * wx + vy_ * wy);
        float acceleration = grounded_ ? 10.0f : settings_.airAcceleration;
        float addition = std::max(0.0f, std::min(needed, acceleration * speed * dt));
        vx_ += addition * wx; vy_ += addition * wy;
    }
    if (jump) {
        // Optional arcade boost is independent of classic strafe gain; Shift/Ctrl suppress it.
        if (settings_.bunnyHop && !input.walk && !input.duck && length > 0) {
            float speed = Speed();
            float boosted = std::min(speed * (1 + settings_.jumpBoostPercent / 100), settings_.maxBunnySpeed * worldScale);
            if (speed > 0 && boosted > speed) { vx_ *= boosted / speed; vy_ *= boosted / speed; }
        }
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
    MovementSurface surface;surface.height=floor;ResolveSurface(surface);
}
void MovementPhysics::ResolveSurface(const MovementSurface& surface) {
    float floor=surface.height;
    if(!std::isfinite(floor))return;
    // Losing support starts a fall at the current height instead of snapping down the cliff.
    if (grounded_ && feetZ_ - floor > settings_.stepHeight) { grounded_ = false; vz_ = 0; return; }
    // Land from above. A verified walkable ramp retains near-ground step tolerance during descent;
    // roofs, props and blocked cliff faces cannot use it to mint another takeoff height.
    float reachable=grounded_ ? feetZ_+settings_.stepHeight : std::max(feetZ_,previousFeetZ_);
    if(surface.walkable && surface.ramp && vz_<=0)reachable=std::max(reachable,feetZ_+settings_.stepHeight);
    if(floor>reachable+.01f || (!grounded_ && (vz_>0 || feetZ_>floor)))return;
    feetZ_=floor;
    if(surface.walkable) {vz_=0;grounded_=true;return;}
    // A steep face is a sliding contact, never ground. Project falling velocity onto its local plane.
    grounded_=false;
    float into=vx_*surface.normal[0]+vy_*surface.normal[1]+vz_*surface.normal[2];
    if(into<0) {vx_-=into*surface.normal[0];vy_-=into*surface.normal[1];vz_-=into*surface.normal[2];}
}
void MovementPhysics::ClipHead(float ceiling) {
    // Standing up in a low passage keeps the previous crouch rather than pushing feet through the floor.
    if(duck_<previousDuck_ && feetZ_+BodyHeight()>ceiling)duck_=previousDuck_;
    if(vz_>0 && previousFeetZ_+(64-28*previousDuck_)*worldScale+12<=ceiling+.01f && feetZ_+BodyHeight()>ceiling) {
        feetZ_=ceiling-BodyHeight();vz_=0;grounded_=false;
    }
}
void MovementPhysics::SlideHorizontal(float nx,float ny) {
    // Retain tangent momentum along walls instead of stopping both axes at a diagonal contact.
    float into=vx_*nx+vy_*ny;
    if(into<0) {vx_-=into*nx;vy_-=into*ny;}
}
