#include "WarcraftCollision.hpp"
#include "DiagnosticLog.hpp"
#include "UnitCollisionProfile.hpp"
#include <algorithm>
#include <cmath>

namespace {
bool Overlaps(const Bounds3& box, float x, float y, float padding) {
    return x + padding > box.minimum[0] && x - padding < box.maximum[0] &&
        y + padding > box.minimum[1] && y - padding < box.maximum[1];
}
float ExitDistance(const Bounds3& box, float x, float y,float radius) {
    // Distance to the closest face of the expanded footprint; smaller means less penetration.
    return std::min({x - box.minimum[0] + radius, box.maximum[0] + radius - x,
        y - box.minimum[1] + radius, box.maximum[1] + radius - y});
}
float ReachableHeight(const MovementPhysics& movement) {
    // Descending sweeps may land on a surface crossed during this frame, never pass through its side.
    return movement.Grounded() ? movement.FeetZ() + movement.StepHeight() :
        std::max(movement.FeetZ(), movement.PreviousFeetZ());
}
bool CliffCell(float x, float y) {
    int level=wc3::GetTerrainCliffLevel(&x,&y);
    // Cliff pathing extends across a 128-unit terrain cell, including diagonal shoreline corners.
    // A half-cell probe misses the lower rim and traps the player immediately after landing.
    for(int point=0;point<8;++point) {
        float angle=point*.78539816339f;
        float sx=x+std::round(std::cos(angle))*128,sy=y+std::round(std::sin(angle))*128;
        if(wc3::GetTerrainCliffLevel(&sx,&sy)!=level) return true;
    }
    return false;
}
}
void WarcraftCollision::Blocked(const char* reason, float x, float y, float oldX, float oldY,
    float bottom, float terrain) const {
    // Share one sampled clock across rejection paths so a stuck player cannot flood the log.
    if (DiagnosticLog::Due(blockedLog_))
        wc3::Trace("movement blocked reason=%s from=%.1f,%.1f sample=%.1f,%.1f reachable=%.1f terrain=%.1f cliff=%d/%d",
            reason,oldX,oldY,x,y,bottom,terrain,
            wc3::GetTerrainCliffLevel(&oldX,&oldY),wc3::GetTerrainCliffLevel(&x,&y));
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
        if (Overlaps(box, x, y, radius_) && box.maximum[2] > bottom + .01f && box.minimum[2] < top) {
            // Recover from existing overlap only while approaching an exit; never deepen it or enter a new solid.
            if (Overlaps(box,oldX,oldY,radius_) && ExitDistance(box,x,y,radius_)<ExitDistance(box,oldX,oldY,radius_)-.001f) continue;
            Blocked("solid-volume",x,y,oldX,oldY,bottom,box.maximum[2]);return false;
        }
    }
    float terrainBottom=bottom, centerTerrain=wc3::TerrainGround(x,y);
    // At takeoff/landing the footprint may meet the uphill side before its center clears it. Keep
    // ordinary step tolerance near ground so a gentle slope does not erase hop momentum.
    if(!movement.Grounded() && std::abs(bottom-centerTerrain)<=movement.StepHeight())
        terrainBottom+=movement.StepHeight();
    for (int point = 0; point < 9; ++point) {
        float angle = (point - 1) * .78539816339f;
        float px = x + (point ? std::cos(angle) * radius_ : 0);
        float py = y + (point ? std::sin(angle) * radius_ : 0);
        float terrain = wc3::TerrainGround(px, py), deck = 0;
        if (!std::isfinite(terrain)) { Blocked("invalid-terrain",px,py,oldX,oldY,bottom,terrain);return false; }
        // After leaving a ledge the trailing footprint still overlaps its lip: permit retreat from
        // existing terrain overlap, but never enter a higher surface or lift the center through it.
        float oldSample = wc3::TerrainGround(oldX + px - x, oldY + py - y);
        if (terrain > terrainBottom + .01f && (!point || terrain > oldSample + .01f)) {
            Blocked("uphill-terrain",px,py,oldX,oldY,bottom,terrain);return false;
        }
        if (!wc3::IsTerrainPathable(&px, &py, 1)) continue;
        if (wc3::WalkableSurface(px, py, deck) && deck <= bottom + .01f) continue;
        bool knownObject = false;
        for (const auto& box : obstacles) if (Overlaps(box, px, py, radius_) &&
            (box.maximum[2] <= bottom + .01f ||
            // Native footprints can exceed the mesh: allow the trailing foot samples to clear a nearby
            // known solid while retreating. The solid-volume check above still rejects entering its mesh.
            (Overlaps(box,oldX,oldY,radius_*2) && ExitDistance(box,x,y,radius_)<ExitDistance(box,oldX,oldY,radius_)-.001f))) knownObject = true;
        if (knownObject) continue;
        // Native cliff cells block walkers even above an edge. Recognize their cliff-level discontinuity,
        // keeping flat unknown blockers closed rather than disabling pathing for every jump.
        if (CliffCell(px,py) || wc3::GetTerrainCliffLevel(&oldX,&oldY)>wc3::GetTerrainCliffLevel(&px,&py)) continue;
        Blocked("native-pathing",px,py,oldX,oldY,bottom,terrain);return false;
    }
    return true;
}
void WarcraftCollision::Move(wc3::Handle unit, MovementPhysics& movement, float dt) {
    dt = std::clamp(dt, 0.f, .05f);
    float x = wc3::Real(wc3::GetUnitX(unit)), y = wc3::Real(wc3::GetUnitY(unit));
    float originalX = x, originalY = y;
    auto body=movement.NativeUnitCollision() ? UnitCollisionProfile::Read(unit,base_) : UnitCollisionProfile{};
    // The actor footprint follows native collision too; unknown profiles retain the conservative legacy radius.
    radius_=body.known && body.colliding ? body.radius : 24;
    float dx = movement.VelocityX() * dt, dy = movement.VelocityY() * dt;
    auto obstacles = obstacles_.Snapshot(unit, x, y, 600 + std::hypot(dx, dy),movement.NativeUnitCollision());
    auto bounds = wc3::GetWorldBounds();
    float minX = wc3::Real(wc3::GetRectMinX(bounds)) + radius_, maxX = wc3::Real(wc3::GetRectMaxX(bounds)) - radius_;
    float minY = wc3::Real(wc3::GetRectMinY(bounds)) + radius_, maxY = wc3::Real(wc3::GetRectMaxY(bounds)) - radius_;
    wc3::RemoveRect(bounds);
    if (std::hypot(dx,dy)<.001f) {
        // Standing still needs floor resolution, but no collision rejection or blocked-movement telemetry.
        movement.ResolveFloor(Floor(x,y,movement,obstacles));return;
    }
    int steps = std::max(1, int(std::ceil(std::hypot(dx, dy) / 8)));
    // Eight-unit swept steps retain thin-wall collision at maximum bunnyhop speed.
    for (int step = 0; step < steps; ++step) {
        float nx = x + dx / steps, ny = y + dy / steps;
        if (nx < minX || nx > maxX) { Blocked("world-boundary",nx,ny,x,y,movement.FeetZ(),0);nx = x; movement.BlockX(); dx = 0; }
        if (ny < minY || ny > maxY) { Blocked("world-boundary",nx,ny,x,y,movement.FeetZ(),0);ny = y; movement.BlockY(); dy = 0; }
        if (Clear(nx, ny, x, y, movement, obstacles)) { x = nx; y = ny; }
        else {
            if (Clear(nx, y, x, y, movement, obstacles)) x = nx; else { movement.BlockX(); dx = 0; }
            if (Clear(x, ny, x, y, movement, obstacles)) y = ny; else { movement.BlockY(); dy = 0; }
        }
        // Follow support after each accepted ground substep. Fast uphill sweeps cannot reuse an outdated frame-start floor.
        // Airborne feet remain controlled by the vertical integrator.
        if(movement.Grounded())movement.ResolveFloor(Floor(x,y,movement,obstacles));
    }
    float floor = Floor(x, y, movement, obstacles);
    if (std::hypot(x - originalX, y - originalY) > 0.001f) {
        bool raised = floor > wc3::TerrainGround(x, y) + .01f;
        float deck = 0;
        bool checkedCliff = wc3::IsTerrainPathable(&x, &y, 1) && CliffCell(x,y) && !wc3::WalkableSurface(x, y, deck);
        bool escapingSolid=false;
        // Do not send a validated retreat back through Warcraft's nearest-walkable-cell search:
        // it may snap the actor into the same obstruction that the swept checks just let it leave.
        for(const auto& box:obstacles) if(Overlaps(box,originalX,originalY,radius_*2) &&
            ExitDistance(box,x,y,radius_)<ExitDistance(box,originalX,originalY,radius_)-.001f) escapingSolid=true;
        if(checkedCliff && floor<movement.FeetZ()-.01f) movement.BeginFall();
        if (!movement.Grounded() || movement.FeetZ() - floor > movement.StepHeight() || raised || checkedCliff || escapingSolid) {
            // XY-only setters preserve the checked air path; SetUnitPosition relocates walkers off cliffs/props.
            wc3::SetUnitX(unit, &x); wc3::SetUnitY(unit, &y);
        } else {
            wc3::SetUnitPosition(unit, &x, &y);
            float actualX = wc3::Real(wc3::GetUnitX(unit)), actualY = wc3::Real(wc3::GetUnitY(unit));
            if (std::hypot(actualX - x, actualY - y) > 2) {
                Blocked("native-relocation",actualX,actualY,originalX,originalY,movement.FeetZ(),floor);
                wc3::SetUnitX(unit, &originalX); wc3::SetUnitY(unit, &originalY); movement.Stop();
            }
        }
    }
    x = wc3::Real(wc3::GetUnitX(unit)); y = wc3::Real(wc3::GetUnitY(unit));
    movement.ResolveFloor(Floor(x, y, movement, obstacles));
}
