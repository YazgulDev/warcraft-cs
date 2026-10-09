#include "WarcraftCollision.hpp"
#include <algorithm>
#include <cmath>

namespace {
constexpr float radius = 24;
bool Overlaps(const Bounds3& box, float x, float y, float padding) {
    return x + padding > box.minimum[0] && x - padding < box.maximum[0] &&
        y + padding > box.minimum[1] && y - padding < box.maximum[1];
}
float ReachableHeight(const MovementPhysics& movement) {
    // Descending sweeps may land on a surface crossed during this frame, never pass through its side.
    return movement.Grounded() ? movement.FeetZ() + movement.StepHeight() :
        std::max(movement.FeetZ(), movement.PreviousFeetZ());
}
bool CliffCell(float x, float y) {
    int level=wc3::GetTerrainCliffLevel(&x,&y);
    // Native cliff levels distinguish real edges from hills, water and arbitrary blocked pathing.
    for(int axis=0;axis<4;++axis) {
        float sx=x+(axis==0 ? 64.f : axis==1 ? -64.f : 0),sy=y+(axis==2 ? 64.f : axis==3 ? -64.f : 0);
        if(wc3::GetTerrainCliffLevel(&sx,&sy)!=level) return true;
    }
    return false;
}
}
float WarcraftCollision::Floor(float x, float y, const MovementPhysics& movement, const std::vector<Bounds3>& obstacles) const {
    float floor = wc3::TerrainGround(x, y), deck = 0;
    float reachable = ReachableHeight(movement);
    if (wc3::WalkableSurface(x, y, deck)) {
        if(deck<=reachable+.01f) floor=std::max(floor,deck);
    } else {
        // GetLocationZ also supplies native shallow-water support. Retain it when no overhead deck
        // is present, rather than lowering a Warcraft walker onto the submerged terrain mesh.
        floor=std::max(floor,wc3::Ground(x,y));
    }
    // A low solid prop can support the feet after landing; moving off its top starts a real fall.
    for (const auto& box : obstacles)
        if (Overlaps(box, x, y, 0) && box.maximum[2] <= reachable + .01f)
            floor = std::max(floor, box.maximum[2]);
    return floor;
}
bool WarcraftCollision::Clear(float x, float y, float oldX, float oldY, const MovementPhysics& movement,
    const std::vector<Bounds3>& obstacles) const {
    float bottom = ReachableHeight(movement), top = movement.EyeZ() + 12;
    for (const auto& box : obstacles) {
        // Height-aware solid volumes prevent flying through buildings/trees, while low props can be cleared.
        if (Overlaps(box, x, y, radius) && box.maximum[2] > bottom + .01f && box.minimum[2] < top) return false;
    }
    float terrainBottom=bottom, centerTerrain=wc3::TerrainGround(x,y);
    // At takeoff/landing the footprint may meet the uphill side before its center clears it. Keep
    // ordinary step tolerance near ground so a gentle slope does not erase hop momentum.
    if(!movement.Grounded() && std::abs(bottom-centerTerrain)<=movement.StepHeight())
        terrainBottom+=movement.StepHeight();
    for (int point = 0; point < 9; ++point) {
        float angle = (point - 1) * .78539816339f;
        float px = x + (point ? std::cos(angle) * radius : 0);
        float py = y + (point ? std::sin(angle) * radius : 0);
        float terrain = wc3::TerrainGround(px, py), deck = 0;
        if (!std::isfinite(terrain)) return false;
        // After leaving a ledge the trailing footprint still overlaps its lip: permit retreat from
        // existing terrain overlap, but never enter a higher surface or lift the center through it.
        float oldSample = wc3::TerrainGround(oldX + px - x, oldY + py - y);
        if (terrain > terrainBottom + .01f && (!point || terrain > oldSample + .01f)) return false;
        if (!wc3::IsTerrainPathable(&px, &py, 1)) continue;
        if (wc3::WalkableSurface(px, py, deck) && deck <= bottom + .01f) continue;
        bool knownObject = false;
        for (const auto& box : obstacles) if (Overlaps(box, px, py, radius) && box.maximum[2] <= bottom + .01f) knownObject = true;
        if (knownObject) continue;
        // Native cliff cells block walkers even above an edge. Recognize their cliff-level discontinuity,
        // keeping flat unknown blockers closed rather than disabling pathing for every jump.
        if (CliffCell(px,py) || wc3::GetTerrainCliffLevel(&oldX,&oldY)>wc3::GetTerrainCliffLevel(&px,&py)) continue;
        return false;
    }
    return true;
}
void WarcraftCollision::Move(wc3::Handle unit, MovementPhysics& movement, float dt) {
    dt = std::clamp(dt, 0.f, .05f);
    float x = wc3::Real(wc3::GetUnitX(unit)), y = wc3::Real(wc3::GetUnitY(unit));
    float originalX = x, originalY = y;
    float dx = movement.VelocityX() * dt, dy = movement.VelocityY() * dt;
    auto obstacles = obstacles_.Snapshot(unit, x, y, 600 + std::hypot(dx, dy));
    auto bounds = wc3::GetWorldBounds();
    float minX = wc3::Real(wc3::GetRectMinX(bounds)) + radius, maxX = wc3::Real(wc3::GetRectMaxX(bounds)) - radius;
    float minY = wc3::Real(wc3::GetRectMinY(bounds)) + radius, maxY = wc3::Real(wc3::GetRectMaxY(bounds)) - radius;
    wc3::RemoveRect(bounds);
    int steps = std::max(1, int(std::ceil(std::hypot(dx, dy) / 8)));
    // Eight-unit swept steps retain thin-wall collision at maximum bunnyhop speed.
    for (int step = 0; step < steps; ++step) {
        float nx = x + dx / steps, ny = y + dy / steps;
        if (nx < minX || nx > maxX) { nx = x; movement.BlockX(); dx = 0; }
        if (ny < minY || ny > maxY) { ny = y; movement.BlockY(); dy = 0; }
        if (Clear(nx, ny, x, y, movement, obstacles)) { x = nx; y = ny; }
        else {
            if (Clear(nx, y, x, y, movement, obstacles)) x = nx; else { movement.BlockX(); dx = 0; }
            if (Clear(x, ny, x, y, movement, obstacles)) y = ny; else { movement.BlockY(); dy = 0; }
        }
    }
    float floor = Floor(x, y, movement, obstacles);
    if (std::hypot(x - originalX, y - originalY) > 0.001f) {
        bool raised = floor > wc3::TerrainGround(x, y) + .01f;
        float deck = 0;
        bool checkedCliff = wc3::IsTerrainPathable(&x, &y, 1) && CliffCell(x,y) && !wc3::WalkableSurface(x, y, deck);
        if(checkedCliff && floor<movement.FeetZ()-.01f) movement.BeginFall();
        if (!movement.Grounded() || movement.FeetZ() - floor > movement.StepHeight() || raised || checkedCliff) {
            // XY-only setters preserve the checked air path; SetUnitPosition relocates walkers off cliffs/props.
            wc3::SetUnitX(unit, &x); wc3::SetUnitY(unit, &y);
        } else {
            wc3::SetUnitPosition(unit, &x, &y);
            float actualX = wc3::Real(wc3::GetUnitX(unit)), actualY = wc3::Real(wc3::GetUnitY(unit));
            if (std::hypot(actualX - x, actualY - y) > 2) {
                wc3::SetUnitX(unit, &originalX); wc3::SetUnitY(unit, &originalY); movement.Stop();
            }
        }
    }
    x = wc3::Real(wc3::GetUnitX(unit)); y = wc3::Real(wc3::GetUnitY(unit));
    movement.ResolveFloor(Floor(x, y, movement, obstacles));
}
