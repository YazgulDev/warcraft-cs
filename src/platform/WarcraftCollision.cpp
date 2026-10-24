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
bool CliffCell(float x, float y, float* barrierHeight=nullptr) {
    int level=wc3::GetTerrainCliffLevel(&x,&y);
    bool cliff=false;
    if(barrierHeight)*barrierHeight=wc3::TerrainGround(x,y);
    // The interpolated face and its blocked lower-bank cell can occupy two terrain cells beyond
    // the native level transition. Probe both rings, including diagonal shoreline corners.
    for(float reach:{128.f,256.f})for(int point=0;point<8;++point) {
        float angle=point*.78539816339f;
        float sx=x+std::round(std::cos(angle))*reach,sy=y+std::round(std::sin(angle))*reach;
        if(wc3::GetTerrainCliffLevel(&sx,&sy)!=level) {
            if(!barrierHeight)return true;
            cliff=true;
        }
        // The native cliff face is visually interpolated. Its neighboring terrace is the barrier,
        // otherwise each tiny swept rise would become another legal step or autojump takeoff.
        if(barrierHeight) {
            float height=wc3::TerrainGround(sx,sy);
            if(std::isfinite(height))*barrierHeight=std::max(*barrierHeight,height);
        }
    }
    return cliff;
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
    // Use the same footprint as the side sweep: landing with a foot over a lip must catch its top
    // before the center crosses. Otherwise gravity sinks the feet below it and the next step hits its side.
    // Only already-reachable terrain can support us; a tall cliff remains a wall during ascent.
    for(int point=0;point<8;++point) {
        float angle=point*.78539816339f;
        float terrain=wc3::TerrainGround(x+std::cos(angle)*radius_,y+std::sin(angle)*radius_);
        if(std::isfinite(terrain) && terrain<=reachable+.01f)floor=std::max(floor,terrain);
    }
    // A low solid prop supports the entire foot too; support ends only after its trailing edge clears.
    for (const auto& box : obstacles)
        if (Overlaps(box, x, y, radius_) && box.maximum[2] <= reachable + .01f)
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
        float cliffTop=terrain;
        bool cliff=CliffCell(px,py,terrain>oldSample+.01f ? &cliffTop : nullptr);
        // Ground step tolerance and near-ground airborne tolerance belong to walkable ramps.
        // A blocked rising cliff requires feet high enough to clear its upper terrace in one jump.
        if(cliff && terrain>oldSample+.01f && cliffTop>bottom+.01f) {
            Blocked("uphill-cliff",px,py,oldX,oldY,bottom,cliffTop);return false;
        }
        bool knownObject = false;
        for (const auto& box : obstacles) if (Overlaps(box, px, py, radius_) &&
            (box.maximum[2] <= bottom + .01f ||
            // Native footprints can exceed the mesh: allow the trailing foot samples to clear a nearby
            // known solid while retreating. The solid-volume check above still rejects entering its mesh.
            (Overlaps(box,oldX,oldY,radius_*2) && ExitDistance(box,x,y,radius_)<ExitDistance(box,oldX,oldY,radius_)-.001f))) knownObject = true;
        if (knownObject) continue;
        // Native cliff cells block walkers even above an edge. Recognize their cliff-level discontinuity,
        // keeping flat unknown blockers closed rather than disabling pathing for every jump.
        if (cliff || wc3::GetTerrainCliffLevel(&oldX,&oldY)>wc3::GetTerrainCliffLevel(&px,&py)) continue;
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
        // Even a clear center can have its native hull on a blocked rim. Commit swept-valid XY
        // throughout this cliff neighborhood rather than invoking nearest-cell placement there.
        bool nearCliff=CliffCell(x,y) && !wc3::WalkableSurface(x,y,deck);
        bool checkedCliff=nearCliff && wc3::IsTerrainPathable(&x,&y,1);
        // Swept collision already owns this local contact. Native nearest-cell placement can publish a
        // sideways sprite snap even when its logical coordinates are immediately rolled back afterward.
        // Preserve exact validated XY for approach, parallel sliding and retreat on either side of a solid.
        bool nearSolid=std::any_of(obstacles.begin(),obstacles.end(),[&](const Bounds3& box) {
            return Overlaps(box,originalX,originalY,radius_*2) || Overlaps(box,x,y,radius_*2);
        });
        if(checkedCliff && floor<movement.FeetZ()-.01f) movement.BeginFall();
        if (!movement.Grounded() || movement.FeetZ() - floor > movement.StepHeight() || raised || nearCliff || nearSolid) {
            // XY-only setters preserve the checked path. Unknown blockers in open space retain native validation below.
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
