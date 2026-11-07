// Restore the owned pre-rework movement adapter from 2babf58; keep later collision in its separate strategy.
#include "ClassicWarcraftCollision.hpp"
#include "DiagnosticLog.hpp"
#include "MovementWorldBounds.hpp"
#include "UnitCollisionProfile.hpp"
#include "MovementWater.hpp"
#include "WalkableTerrainContact.hpp"
#include "TerrainSurfContact.hpp"
#include "ModelSurfContact.hpp"
#include "AirbornePathing.hpp"
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
    // The interpolated face and its blocked lower-bank cell can occupy two terrain cells beyond
    // the native level transition. Probe both rings, including diagonal shoreline corners.
    for(float reach:{128.f,256.f})for(int point=0;point<8;++point) {
        float angle=point*.78539816339f;
        float sx=x+std::round(std::cos(angle))*reach,sy=y+std::round(std::sin(angle))*reach;
        if(wc3::GetTerrainCliffLevel(&sx,&sy)!=level) return true;
    }
    return false;
}
}
void ClassicWarcraftCollision::Blocked(const char* reason, float x, float y, float oldX, float oldY,
    float bottom, float terrain) const {
    // Share one sampled clock across rejection paths so a stuck player cannot flood the log.
    if (DiagnosticLog::Due(blockedLog_))
        wc3::Trace("movement blocked reason=%s from=%.1f,%.1f sample=%.1f,%.1f reachable=%.1f terrain=%.1f cliff=%d/%d",
            reason,oldX,oldY,x,y,bottom,terrain,
            wc3::GetTerrainCliffLevel(&oldX,&oldY),wc3::GetTerrainCliffLevel(&x,&y));
}
float ClassicWarcraftCollision::Floor(float x, float y, const MovementPhysics& movement, const std::vector<Bounds3>& obstacles) const {
    float floor = wc3::TerrainGround(x, y), deck = 0;
    float reachable = ReachableHeight(movement);
    if (wc3::WalkableSurface(x, y, deck)) {
        if(deck<=reachable+.01f) floor=std::max(floor,deck);
    } else {
        // Liquid-aware support uses the actual bed; an explicit swimming opt-out retains legacy water support.
        floor=std::max(floor,MovementWater::Ground(x,y,movement));
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
    for (size_t index=0;index<obstacles.size();++index) {
        const auto& box=obstacles[index];if(!Overlaps(box,x,y,radius_))continue;
        bool rock = obstacles_.IsMapRock(index);
        const auto* mesh = rock ? obstacles_.Geometry(index) : surfMeshes_[index];
        if(mesh) {
            float height=0;
            float reach = rock ? MapRockObstacles::ReachableHeight(box, movement.Grounded(), movement.FeetZ(), reachable) : reachable;
            if(mesh->Floor(x,y,radius_,reach,.0001f,height,nullptr,false))floor=std::max(floor,height);
        } else if(box.maximum[2]<=reachable+.01f)floor=std::max(floor,box.maximum[2]);
    }
    return floor;
}
bool ClassicWarcraftCollision::Clear(float x, float y, float oldX, float oldY, const MovementPhysics& movement,
    const std::vector<Bounds3>& obstacles) const {
    float bottom = ReachableHeight(movement), top = movement.EyeZ() + 12;
    for (size_t index=0;index<obstacles.size();++index) {
        const auto& box=obstacles[index];
        // A rock's actual mesh blocks the body before cliff pathing exceptions are evaluated.
        bool rock = obstacles_.IsMapRock(index);
        const auto* mesh = rock ? obstacles_.Geometry(index) : surfMeshes_[index];
        if(mesh) {
            if(rock && !Overlaps(box,x,y,radius_))continue;
            float reach = rock ? MapRockObstacles::ReachableHeight(box, movement.Grounded(), movement.FeetZ(), bottom) : bottom;
            float normal[2]={};
            if(!mesh->Contact(x,y,radius_,reach,top,normal))continue;
            // Outward retreat from an existing overlap is legal; entering any solid face is not.
            if(mesh->Contact(oldX,oldY,radius_,movement.FeetZ(),movement.FeetZ()+movement.BodyHeight()) &&
                (x-oldX)*normal[0]+(y-oldY)*normal[1]>.001f)continue;
            Blocked(rock ? "map-rock" : "surf-solid-mesh",x,y,oldX,oldY,reach,box.maximum[2]);return false;
        }
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
        if(MovementWater::Passable(px,py,movement))continue;
        bool knownObject = false;
        for(size_t index=0;index<obstacles.size();++index) {
            const auto& box=obstacles[index];
            // Once the mesh has cleared the body, its pathing cell must not fill an empty corner.
            if((obstacles_.IsMapRock(index) || surfMeshes_[index]) && ModelSurfContact::PathCell(box,px,py)){knownObject=true;continue;}
            if (Overlaps(box, px, py, radius_) &&
            (box.maximum[2] <= bottom + .01f ||
            // Native footprints can exceed the mesh: allow the trailing foot samples to clear a nearby
            // known solid while retreating. The solid-volume check above still rejects entering its mesh.
            (Overlaps(box,oldX,oldY,radius_*2) && ExitDistance(box,x,y,radius_)<ExitDistance(box,oldX,oldY,radius_)-.001f))) knownObject = true;
        }
        if (knownObject) continue;
        // Native cliff cells block walkers even above an edge. Recognize their cliff-level discontinuity,
        // keeping flat unknown blockers closed rather than disabling pathing for every jump.
        if (CliffCell(px,py) || wc3::GetTerrainCliffLevel(&oldX,&oldY)>wc3::GetTerrainCliffLevel(&px,&py)) continue;
        if (AirbornePathing::CanOverfly(px, py, terrain, movement)) continue;
        Blocked("native-pathing",px,py,oldX,oldY,bottom,terrain);return false;
    }
    return true;
}
bool ClassicWarcraftCollision::RampContact(float x,float y,MovementPhysics& movement,const std::vector<Bounds3>& obstacles) const {
    if(!movement.ClassicRampContacts() || movement.Grounded() || movement.Swimming() || movement.Wet())return false;
    MovementSurface best;best.walkable=false;
    float reach=std::max(movement.FeetZ(),movement.PreviousFeetZ())+movement.StepHeight();
    for(int point=0;point<9;++point) {
        float angle=(point-1)*.78539816339f,px=x+(point ? std::cos(angle)*radius_ : 0),py=y+(point ? std::sin(angle)*radius_ : 0);
        float height=wc3::TerrainGround(px,py);
        if(height<movement.FeetZ()-.01f || height>reach+.01f)continue;
        auto surface=WalkableTerrainContact::Sample(px,py,movement.MaxWalkSlope(),movement.VelocityX(),movement.VelocityY());
        float into=movement.VelocityX()*surface.normal[0]+movement.VelocityY()*surface.normal[1]+movement.VerticalVelocity()*surface.normal[2];
        if(surface.walkable && into<-.001f && (!best.walkable || surface.height>best.height))best=surface;
    }
    if(!best.walkable)return false;
    // A terrain correction cannot lift the body through a model or the underside of a native deck.
    float top=best.height+movement.BodyHeight();
    for(const auto& box:obstacles)if(Overlaps(box,x,y,radius_) && box.maximum[2]>best.height+.01f && box.minimum[2]<top-.01f)return false;
    for(int point=0;point<9;++point) {
        float angle=(point-1)*.78539816339f,deck=0;
        float px=x+(point ? std::cos(angle)*radius_ : 0),py=y+(point ? std::sin(angle)*radius_ : 0);
        if(wc3::WalkableSurface(px,py,deck) && deck>movement.FeetZ()+.01f && deck<top-.01f)return false;
    }
    return movement.ResolveClassicRamp(best);
}
void ClassicWarcraftCollision::Move(wc3::Handle unit, MovementPhysics& movement, float dt) {
    // Award a queued takeoff bonus only after all support and native placement checks.
    struct CommitJump {MovementPhysics& motion;~CommitJump(){motion.FinishMove();}} commit{movement};
    MovementWater::Update(unit,movement);
    dt = std::clamp(dt, 0.f, .05f);
    float x = wc3::Real(wc3::GetUnitX(unit)), y = wc3::Real(wc3::GetUnitY(unit));
    float originalX = x, originalY = y;
    auto body=movement.NativeUnitCollision() ? UnitCollisionProfile::Read(unit,base_) : UnitCollisionProfile{};
    // The actor footprint follows native collision too; unknown profiles retain the conservative legacy radius.
    radius_=body.known && body.colliding ? body.radius : 24;
    auto bounds=MovementWorldBounds::Read(radius_);
    bounds.Constrain(unit,movement,x,y);
    originalX=x;originalY=y;
    float minX=bounds.minX,maxX=bounds.maxX,minY=bounds.minY,maxY=bounds.maxY;
    float dx = movement.VelocityX() * dt, dy = movement.VelocityY() * dt;
    auto range=ModelSurfContact::Range(movement);
    auto obstacles = obstacles_.Snapshot(unit, x, y, 600 + std::hypot(dx, dy),movement.NativeUnitCollision(),movement.SurfModelContacts(),false,range,true);
    surfMeshes_.clear();
    for(size_t index=0;index<obstacles.size();++index) {
        const auto* mesh=obstacles_.Geometry(index);
        // Ordinary rock doodads provide solid contact without creating new surf ramps.
        surfMeshes_.push_back(mesh && !obstacles_.IsMapRock(index) && mesh->HasSurface(range) ? mesh : nullptr);
    }
    // Imported surf ramps have real undersides too; scope head clipping to these eligible meshes.
    auto ceiling=[&](float px,float py) {
        for(const auto* mesh:surfMeshes_)if(mesh) {
            float height=0;if(mesh->Ceiling(px,py,radius_,movement.PreviousFeetZ(),height))movement.ClipHead(height-.01f);
        }
    };
    ceiling(x,y);
    auto support=[&]() {
        float floor=Floor(x,y,movement,obstacles);
        if(!ModelSurfContact::OwnsFloor(x,y,radius_,floor,movement,surfMeshes_) &&
            !TerrainSurfContact::OwnsFloor(x,y,radius_,floor,movement))movement.ResolveClassicFloor(floor);
    };
    if (std::hypot(dx,dy)<.001f) {
        // Standing still needs floor resolution, but no collision rejection or blocked-movement telemetry.
        support();
        auto before=movement;
        bool contact=ModelSurfContact::Resolve(x,y,radius_,movement,surfMeshes_) || TerrainSurfContact::Resolve(x,y,radius_,movement,obstacles);
        if(contact && !Clear(x,y,x,y,movement,obstacles))movement=before;
        MovementWater::Update(unit,movement);return;
    }
    // The vertical integrator may have just reached the floor at the current XY. Settle that
    // existing classic contact before testing the first horizontal step, not after clipping it.
    support();
    auto settle=[&](float targetX,float targetY,float fromX,float fromY) {
        // A rejected candidate must not leave its height/ground state behind at the old XY.
        auto before=movement;
        ceiling(targetX,targetY);
        if(!ModelSurfContact::Resolve(targetX,targetY,radius_,movement,surfMeshes_) &&
            !TerrainSurfContact::Resolve(targetX,targetY,radius_,movement,obstacles))RampContact(targetX,targetY,movement,obstacles);
        if(Clear(targetX,targetY,fromX,fromY,movement,obstacles))return true;
        movement=before;return false;
    };
    settle(x,y,x,y);
    dx=movement.VelocityX()*dt;dy=movement.VelocityY()*dt;
    // Surf may convert falling velocity into XY; bound sweeps by total incoming motion.
    int steps=std::max(1,int(std::ceil(std::hypot(std::hypot(dx,dy),movement.VerticalVelocity()*dt)/8)));
    // Eight-unit swept steps retain thin-wall collision at maximum bunnyhop speed.
    for (int step = 0; step < steps; ++step) {
        float nx = x + dx / steps, ny = y + dy / steps;
        if (nx < minX || nx > maxX) { Blocked("world-boundary",nx,ny,x,y,movement.FeetZ(),0);nx = x; movement.BlockX(); dx = 0; }
        if (ny < minY || ny > maxY) { Blocked("world-boundary",nx,ny,x,y,movement.FeetZ(),0);ny = y; movement.BlockY(); dy = 0; }
        if (settle(nx, ny, x, y)) { x = nx; y = ny; }
        else {
            if (settle(nx, y, x, y)) x = nx; else { movement.BlockX(); dx = 0; }
            if (settle(x, ny, x, y)) y = ny; else { movement.BlockY(); dy = 0; }
        }
        // Resolve descending landings as well as ground support at each swept substep. Delaying an
        // accepted ramp landing until frame end leaves stale airborne feet and falsely clips the next step.
        // Classic's existing resolver still rejects ascending landings and starts genuine ledge falls.
        support();
        dx=movement.VelocityX()*dt;dy=movement.VelocityY()*dt;
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
    bounds.Constrain(unit,movement,x,y);
    support();
    MovementWater::Update(unit,movement);
}
