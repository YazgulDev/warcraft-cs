#include "MovementPhysics.hpp"
#include <algorithm>
#include <cmath>

void MovementPhysics::Reset(float floor) {
    surfing_=false;pendingJumpBoost_=0;
    water_={};swimming_=waterJump_=false;waterAllowed_=true; // Map, actor and cargo resets cannot retain another pool's buoyancy.
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
    // Portable callers may omit an explicit commit; the previous resolved frame is still finalized once.
    // Consume only the last confirmed face contact, once per simulation step. Repeated
    // swept contact queries cannot multiply acceleration, and free-air bunnyhop stays unchanged.
    bool surfInput=surfing_ && settings_.surf.enabled && !grounded_ && !swimming_;
    FinishMove();surfing_=false;
    dt = std::clamp(dt, 0.0f, 0.05f);
    if (!dt) return;
    previousFeetZ_ = feetZ_;
    previousDuck_=duck_;
    if(swimming_) {
        if(input.waterExit && input.jump && !jumpHeld_ && !jumpSuppressed_ && !input.immobilized && settings_.swimming.exitHeight>0) {
            // A fresh Space at a bank gets a bounded ballistic exit. Repeated contact updates must
            // not replace its ascent with buoyancy; no ground/bunnyhop impulse is awarded.
            float rise=std::max(0.f,water_.surface+settings_.swimming.exitHeight-feetZ_);
            vz_=std::sqrt(2*settings_.gravity*worldScale*rise);
            swimming_=false;waterJump_=true;jumpHeld_=true;
            // A shore exit needs forward travel even when the wall has already stopped paddling.
            // Replace, rather than add to, horizontal speed so repeated exits cannot mint momentum.
            float speed=std::max(0.f,weaponSpeed)*worldScale*settings_.swimming.speedPercent/100;
            float length=std::hypot(input.forward,input.right),angle=input.yaw*.01745329252f;
            if(length>.01f) {
                vx_=speed*(input.forward*std::cos(angle)+input.right*std::sin(angle))/length;
                vy_=speed*(input.forward*std::sin(angle)-input.right*std::cos(angle))/length;
            } else LimitSpeed(speed);
        } else {Swim(input,dt,weaponSpeed);return;}
    }
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
        float surfGain=surfInput ? settings_.surf.accelerationMultiplier : 1;
        float cap = grounded_ ? speed : std::min(speed, 30 * worldScale * surfGain);
        float needed = cap - (vx_ * wx + vy_ * wy);
        float acceleration = grounded_ ? 10.0f : settings_.airAcceleration * surfGain;
        float addition = std::max(0.0f, std::min(needed, acceleration * speed * dt));
        vx_ += addition * wx; vy_ += addition * wy;
    }
    if (jump) {
        // Optional arcade boost is independent of classic strafe gain; Shift/Ctrl suppress it.
        if (settings_.bunnyHop && !input.walk && !input.duck && length > 0) {
            pendingJumpBoost_=settings_.jumpBoostPercent/100;
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
void MovementPhysics::FinishMove() {
    float bonus=pendingJumpBoost_;pendingJumpBoost_=0;
    // Ramp recontacts and ceiling hits are not a free-flight jump. Repeated takeoff/landing
    // within one frame must never multiply the arcade bonus at the rendering frame rate.
    if(bonus<=0 || grounded_ || swimming_ || surfing_ || vz_<=0)return;
    float speed=Speed(),boosted=std::min(speed*(1+bonus),settings_.maxBunnySpeed*worldScale);
    if(speed>0 && boosted>speed){vx_*=boosted/speed;vy_*=boosted/speed;}
}
bool MovementPhysics::ResolveSurf(const MovementSurface& surface) {
    if(!settings_.surf.enabled || swimming_ || !std::isfinite(surface.height))return false;
    float z=surface.normal[2];
    if(!std::isfinite(z) || z<=std::cos(settings_.surf.maximumAngle*.01745329252f) ||
        z>=std::cos(settings_.maxWalkSlope*.01745329252f) ||
        surface.height>std::max(feetZ_,previousFeetZ_)+settings_.stepHeight+.01f ||
        feetZ_-surface.height>settings_.surf.contactDistance+.01f)return false;
    float into=vx_*surface.normal[0]+vy_*surface.normal[1]+vz_*z;
    if(!std::isfinite(into) || into>.001f)return false; // Moving away from the face releases the contact.
    // Remove only velocity into the plane. Gravity/air input supply motion; the contact adds no energy.
    vx_-=std::min(0.f,into)*surface.normal[0];vy_-=std::min(0.f,into)*surface.normal[1];vz_-=std::min(0.f,into)*z;
    float speed=Speed(),cap=settings_.maxBunnySpeed*worldScale;
    if(speed>cap){float scale=cap/speed;vx_*=scale;vy_*=scale;vz_*=scale;}
    feetZ_=surface.height;grounded_=false;surfing_=true;pendingJumpBoost_=0;return true;
}
float MovementPhysics::FloatFeet() const {
    return std::max(water_.bottom,water_.surface-(64-28*duck_)*worldScale+settings_.swimming.eyeAboveWater);
}
void MovementPhysics::SetWater(const WaterContact& water) {
    bool wasSwimming=swimming_;
    bool valid=water.present && std::isfinite(water.surface) && std::isfinite(water.bottom) && water.surface>water.bottom;
    water_=valid ? water : WaterContact{}; // Rejected contact cannot leave a bogus wet flag or infinite water level.
    // Keep the one exit impulse until landing or descending back to the floating body level.
    if(waterJump_ && SwimmingEnabled() && valid && !grounded_ && (vz_>0 || feetZ_>FloatFeet()+.01f)) {
        swimming_=false;return;
    }
    waterJump_=false;
    // A small internal hysteresis avoids toggling at the deep/shallow boundary. Actual deck support
    // above the float body retains walking; airborne actors enter only when they reach the water.
    float threshold=settings_.swimming.minimumDepth-(wasSwimming ? std::min(12.f,settings_.swimming.minimumDepth*.1f) : 0);
    swimming_=SwimmingEnabled() && valid && water.surface-water.bottom>=threshold &&
        feetZ_<=water.surface+settings_.stepHeight && (!grounded_ || feetZ_<=FloatFeet()+settings_.stepHeight);
    if(swimming_) {grounded_=false;if(!wasSwimming)vz_=0;}
    else if(wasSwimming)vz_=0; // Returning to shore resumes ordinary gravity without a buoyancy impulse.
}
void MovementPhysics::Swim(const MoveInput& input,float dt,float weaponSpeed) {
    const auto& water=settings_.swimming;
    // Surface swimming has no ground takeoff, bunnyhop gain or airborne strafe acceleration.
    // Stuns/roots stop paddling but do not pin the body below the surface.
    if(!input.stanceLocked)duck_+=std::clamp(-duck_,-dt/.2f,dt/.2f);
    jumpHeld_=input.jump;if(!input.jump)jumpSuppressed_=false;else if(input.immobilized)jumpSuppressed_=true;
    if(input.immobilized)Stop();
    float drag=std::exp(-water.drag*dt);vx_*=drag;vy_*=drag;
    float speed=std::max(0.f,weaponSpeed)*worldScale*water.speedPercent/100;
    if(input.walk)speed*=.52f;
    float length=input.immobilized ? 0 : std::hypot(input.forward,input.right);
    if(length>0) {
        float angle=input.yaw*.01745329252f;
        float wx=(input.forward*std::cos(angle)+input.right*std::sin(angle))/length;
        float wy=(input.forward*std::sin(angle)-input.right*std::cos(angle))/length;
        float add=std::clamp(speed-vx_*wx-vy_*wy,0.f,water.acceleration*speed*dt);
        vx_+=wx*add;vy_+=wy*add;
    }
    LimitSpeed(speed);
    float change=(FloatFeet()-feetZ_)*(1-std::exp(-dt/water.floatSeconds));
    feetZ_+=change;vz_=change/dt;grounded_=false;
}
void MovementPhysics::ResolveFloor(float floor) {
    MovementSurface surface;surface.height=floor;ResolveSurface(surface);
}
void MovementPhysics::ResolveClassicFloor(float floor) {
    // The original landing/step behavior belongs to classic collision only; do not apply
    // later slope, ceiling or reachable-height rules to the restored movement strategy.
    if(grounded_ && feetZ_-floor>settings_.stepHeight){grounded_=false;vz_=0;return;}
    if(grounded_ || (vz_<=0 && feetZ_<=floor)){feetZ_=floor;vz_=0;grounded_=true;}
}
bool MovementPhysics::ResolveClassicRamp(const MovementSurface& surface) {
    // Rising in world Z can still move INTO an uphill plane. Only a real, bounded foot contact
    // becomes ground; no velocity is minted and a departing jump cannot stick back to the ramp.
    if(!settings_.classicCollision || !settings_.classicRampContacts || grounded_ || swimming_ ||
        !surface.walkable || !surface.ramp || !std::isfinite(surface.height) ||
        surface.height<feetZ_-.01f || surface.height>std::max(feetZ_,previousFeetZ_)+settings_.stepHeight+.01f)return false;
    float into=vx_*surface.normal[0]+vy_*surface.normal[1]+vz_*surface.normal[2];
    if(!std::isfinite(into) || into>=-.001f)return false;
    feetZ_=surface.height;vz_=0;grounded_=true;return true;
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
