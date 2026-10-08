#include "WarcraftCollision.hpp"
#include <algorithm>
#include <cmath>

bool WarcraftCollision::Clear(float x, float y, float oldFloor) const {
    // Check a player-sized footprint, not a point that can fit inside a wall or tree.
    constexpr float radius = 24, stepHeight = 27;
    for (int point = 0; point < 9; ++point) {
        float angle = (point - 1) * 0.78539816339f;
        float px = x + (point ? std::cos(angle) * radius : 0);
        float py = y + (point ? std::sin(angle) * radius : 0);
        if (wc3::IsTerrainPathable(&px, &py, 1)) {
            // Terrain below a living bridge is blocked, but its native walkable deck is a floor.
            // Query every footprint point; dead bridges, trees and deck edges cannot grant passage.
            float deck = 0;
            if (!wc3::WalkableSurface(px,py,deck) || std::abs(deck-oldFloor)>stepHeight) return false;
        }
    }
    // Warcraft cliffs are vertical walls; only small steps and ramps are traversable.
    return std::abs(wc3::Ground(x, y) - oldFloor) <= stepHeight;
}
void WarcraftCollision::Move(wc3::Handle unit, MovementPhysics& movement, float dt) const {
    float x = wc3::Real(wc3::GetUnitX(unit)), y = wc3::Real(wc3::GetUnitY(unit));
    float dx = movement.VelocityX() * dt, dy = movement.VelocityY() * dt;
    int steps = std::max(1, int(std::ceil(std::hypot(dx, dy) / 8)));
    // Swept substeps stop a fast player tunnelling across thin pathing obstacles.
    for (int step = 0; step < steps; ++step) {
        float floor = wc3::Ground(x, y), nx = x + dx / steps, ny = y + dy / steps;
        if (Clear(nx, ny, floor)) { x = nx; y = ny; }
        else {
            bool movedX = Clear(nx, y, floor);
            if (movedX) x = nx; else { movement.BlockX(); dx = 0; }
            if (Clear(x, ny, floor)) y = ny; else { movement.BlockY(); dy = 0; }
        }
    }
    float originalX = wc3::Real(wc3::GetUnitX(unit)), originalY = wc3::Real(wc3::GetUnitY(unit));
    if (std::hypot(x - originalX, y - originalY) > 0.001f) {
        // Keep native unit/building collision authoritative and discard velocity on relocation.
        wc3::SetUnitPosition(unit, &x, &y);
        float actualX = wc3::Real(wc3::GetUnitX(unit)), actualY = wc3::Real(wc3::GetUnitY(unit));
        if (std::hypot(actualX - x, actualY - y) > 2) {
            // SetUnitPosition can search for a different free point across a wall.
            // Reject that relocation rather than letting the camera cross the tested sweep.
            wc3::SetUnitPosition(unit, &originalX, &originalY); movement.Stop();
        }
    }
    movement.ResolveFloor(wc3::Ground(wc3::Real(wc3::GetUnitX(unit)), wc3::Real(wc3::GetUnitY(unit))));
}
