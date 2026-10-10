#include "../src/movement/MovementPhysics.hpp"
#include "MovementStrafeInput.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static void Require(bool condition, const char* description) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", description); std::exit(1); }
}
static float JumpPeak(int fps) {
    MovementPhysics movement; movement.Reset(320);
    MovementSettings settings; settings.autoJump = false; movement.Configure(settings);
    MoveInput input; input.jump = true;
    float peak = 320;
    for (int frame = 0; frame < fps * 2; ++frame) {
        movement.Step(input, 1.0f / fps, 250); movement.ResolveFloor(320);
        peak = std::fmax(peak, movement.FeetZ());
        Require(movement.FeetZ() >= 320, "jump must not penetrate the floor");
    }
    Require(movement.Grounded(), "holding Space must not automatically repeat jumps");
    return peak;
}
static float StrafeSpeed(int fps, bool enabled = true, float acceleration = 10) {
    MovementPhysics movement; movement.Reset(0);
    MovementSettings settings; settings.bunnyHop = enabled; settings.airAcceleration = acceleration;
    movement.Configure(settings);
    MoveInput run; run.forward = 1;
    for (int frame = 0; frame < fps; ++frame) movement.Step(run, 1.0f / fps, 250);
    int hops = 0;
    for (int frame = 0; frame < fps * 8; ++frame) {
        MoveInput input = MovementStrafeInput(movement, float(frame) / fps);
        bool grounded = movement.Grounded();
        movement.Step(input, 1.0f / fps, 250); movement.ResolveFloor(0);
        if (grounded && !movement.Grounded()) ++hops;
        Require(movement.Speed() <= 1500.01f, "strafing must respect the configured horizontal cap");
    }
    Require(hops >= 10, "held Space must chain real takeoffs through repeated landings");
    return movement.Speed();
}
int main() {
    // Validate player-visible invariants, including frame-rate independence and diagonal speed.
    MovementPhysics straight, diagonal; straight.Reset(100); diagonal.Reset(100);
    MoveInput forward, both; forward.forward = 1; both.forward = both.right = 1;
    for (int frame = 0; frame < 120; ++frame) {
        straight.Step(forward, 1.0f / 60, 250); diagonal.Step(both, 1.0f / 60, 250);
    }
    Require(std::abs(straight.Speed() - 375) < 0.01f, "weapon speed must scale into Warcraft units");
    Require(std::abs(diagonal.Speed() - straight.Speed()) < 0.01f, "diagonals must not run faster");
    for (int frame = 0; frame < 60; ++frame) straight.Step({}, 1.0f / 60, 250);
    Require(straight.Speed() < 0.01f, "releasing movement keys must brake to rest");
    MoveInput duck; duck.forward = 1; duck.duck = true;
    for (int frame = 0; frame < 120; ++frame) straight.Step(duck, 1.0f / 60, 250);
    Require(std::abs(straight.EyeZ() - 154) < 0.01f, "ducked eye must stay above the floor");
    Require(straight.Speed() <= 375 * 0.34f + 0.01f, "duck must reduce maximum speed");
    float peak30 = JumpPeak(30), peak144 = JumpPeak(144);
    Require(peak30 > 386 && peak30 < 389, "jump must reach the expected height");
    Require(std::abs(peak30 - peak144) < 0.3f, "jump height must remain stable across frame rates");
    MovementPhysics airborne; airborne.Reset(0); MoveInput jump; jump.jump = true; jump.right = 1;
    airborne.Step(jump, 0.016f, 250);
    airborne.BlockY(); // Isolate the airborne wish-direction cap from grounded takeoff acceleration.
    airborne.Step(jump, 0.016f, 250);
    Require(std::abs(airborne.VelocityY()) <= 45.01f, "air acceleration must cap the wish-direction gain");
    // Status effects must stop momentum immediately and cannot be bypassed by jumping or air strafing.
    MovementPhysics rooted; rooted.Reset(0);
    for (int frame = 0; frame < 30; ++frame) rooted.Step(forward, 1.0f / 60, 250);
    MoveInput bound = forward; bound.jump = true; bound.immobilized = true;
    rooted.Step(bound, 1.0f / 60, 250);
    Require(rooted.Speed() == 0 && rooted.Grounded(), "root must stop inertia and block jump");
    bound.immobilized = false; rooted.Step(bound, 1.0f / 60, 250);
    Require(rooted.Grounded(), "Space pressed while rooted must not become a queued jump");
    bound.jump = false; rooted.Step(bound, 1.0f / 60, 250);
    bound.jump = true; rooted.Step(bound, 1.0f / 60, 250);
    Require(!rooted.Grounded(), "new jump input must work after root expires");
    bound.immobilized = true;
    for (int frame = 0; frame < 90; ++frame) { rooted.Step(bound, 1.0f / 60, 250); rooted.ResolveFloor(0); }
    Require(rooted.Grounded() && rooted.Speed() == 0, "airborne stun/root must fall and land without drifting");
    MoveInput partialDuck; partialDuck.duck = true;
    rooted.Step(partialDuck, 0.05f, 250); float heldStance = rooted.Duck();
    partialDuck.stanceLocked = partialDuck.immobilized = true;
    rooted.Step(partialDuck, 0.05f, 250);
    Require(rooted.Duck() == heldStance, "stun must freeze an in-progress crouch");
    MovementPhysics slowed; slowed.Reset(0);
    for (int frame = 0; frame < 60; ++frame) slowed.Step(forward, 1.0f / 60, 250);
    slowed.LimitSpeed(187.5f);
    Require(std::abs(slowed.Speed() - 187.5f) < 0.01f, "slow must immediately constrain existing speed");
    for (int frame = 0; frame < 60; ++frame) slowed.Step(forward, 1.0f / 60, 125);
    Require(std::abs(slowed.Speed() - 187.5f) < 0.01f, "slow must persist while running");
    slowed.Step(jump, 1.0f / 60, 125); slowed.LimitSpeed(187.5f);
    Require(slowed.Speed() <= 187.51f, "jump/strafe must not bypass the slow's speed limit");
    // Straight hops retain the run-up; acceleration requires changing the airborne wish direction.
    MovementPhysics hopping; hopping.Reset(0);
    for (int frame = 0; frame < 60; ++frame) hopping.Step(forward, 1.f/60, 250);
    MoveInput hop = forward; hop.jump = true;
    for (int frame = 0; frame < 360; ++frame) { hopping.Step(hop, 1.f/60, 250); hopping.ResolveFloor(0); }
    Require(std::abs(hopping.Speed() - 375) < .01f, "straight held-Space hops must preserve speed without an automatic bonus");
    Require(hopping.Speed() <= 1500.01f, "bunnyhop must remain under the configured speed cap");
    const int frameRates[] = {30, 60, 144};
    for (int fps : frameRates) {
        float speed = StrafeSpeed(fps);
        std::printf("Air-strafe %d FPS: running=375 final=%.1f Warcraft units/s after 8 seconds\n", fps, speed);
        Require(speed > 375 * 1.8f, "alternating A/D with smooth turns must accelerate above running speed");
    }
    Require(StrafeSpeed(60, true, 0) <= 375.01f, "zero air acceleration must disable strafe gain even on takeoff frames");
    Require(StrafeSpeed(60, false) < 450, "disabling bunnyhop must remove accumulated excess speed on each takeoff");
    // A fresh forward jump must use the air-direction cap from its first frame, not ground acceleration.
    MovementPhysics launch; launch.Reset(0); MoveInput launchInput = forward; launchInput.jump = true;
    launch.Step(launchInput, 1.f/30, 250);
    Require(launch.Speed() <= 45.01f, "takeoff must not grant a ground acceleration impulse");
    // Existing custom arcade boosts remain opt-in and survive live tuning changes.
    MovementSettings arcade; arcade.jumpBoostPercent = 8; hopping.Configure(arcade);
    for (int frame = 0; frame < 180; ++frame) { hopping.Step(hop, 1.f/60, 250); hopping.ResolveFloor(0); }
    Require(hopping.Speed() > 450, "explicit takeoff boost must retain its configurable behavior");
    MovementPhysics still; still.Reset(0); MoveInput space; space.jump = true;
    for (int frame = 0; frame < 120; ++frame) { still.Step(space, 1.f/60, 250); still.ResolveFloor(0); }
    Require(still.Speed() == 0, "standing jumps cannot mint horizontal velocity");
    // A drop loses support without teleporting; gravity lands at the lower surface after multiple frames.
    MovementPhysics falling; falling.Reset(400); falling.ResolveFloor(0);
    Require(!falling.Grounded() && falling.FeetZ() == 400, "cliff departure preserves the starting height");
    falling.Step({}, 1.f/60, 250); falling.ResolveFloor(0);
    Require(falling.FeetZ() < 400 && falling.FeetZ() > 390, "cliff fall must integrate gravity gradually");
    for (int frame = 0; frame < 90; ++frame) { falling.Step({}, 1.f/60, 250); falling.ResolveFloor(0); }
    Require(falling.Grounded() && falling.FeetZ() == 0, "cliff fall must land without penetrating terrain");
    // F8-style tuning replacement preserves momentum/position; the next step consumes the new cap.
    MovementSettings tuned; tuned.maxBunnySpeed = 300; tuned.jumpBoostPercent = 0;
    float feet = hopping.FeetZ(); hopping.Configure(tuned);
    Require(hopping.FeetZ() == feet, "live reload must not reset jump height");
    hopping.Step(hop, 1.f/60, 250);
    Require(hopping.Speed() <= 450.01f, "live cap changes must affect the next simulation step");
    std::puts("Movement invariants passed: running, stance, jumping, air strafing, bunnyhop, cap, falling, reload and status effects");
}
