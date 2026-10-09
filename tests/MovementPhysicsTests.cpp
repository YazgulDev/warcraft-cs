#include "../src/movement/MovementPhysics.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static void Require(bool condition, const char* description) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", description); std::exit(1); }
}
static float JumpPeak(int fps) {
    MovementPhysics movement; movement.Reset(320);
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
    std::puts("Movement invariants passed: speed, diagonal, braking, duck, jump, air acceleration, root, stun gravity, slow");
}
