#include "WarcraftCollision.hpp"
#include "DiagnosticLog.hpp"
#include "UnitCollisionProfile.hpp"
#include "../geometry/CylinderHull.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
bool Overlaps(const Bounds3& box, float x, float y, float padding) {
    return CylinderHull::Overlaps(box,x,y,padding);
}
float ExitDistance(const Bounds3& box, float x, float y,float radius) {
    // Distance to the closest face of the expanded footprint; smaller means less penetration.
    return std::min({x - box.minimum[0] + radius, box.maximum[0] + radius - x,
        y - box.minimum[1] + radius, box.maximum[1] + radius - y});
}
bool ModelPathCell(const Bounds3& box,float x,float y) {
    // Attribute only the actual native 32-unit cell intersecting this known solid, not a padded region.
    float left=std::floor(x/32)*32,bottom=std::floor(y/32)*32;
    return left<box.maximum[0] && left+32>box.minimum[0] &&
        bottom<box.maximum[1] && bottom+32>box.minimum[1];
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
void WarcraftCollision::Blocked(const char* reason, float x, float y, float oldX, float oldY,
    float bottom, float terrain) const {
    // Share one sampled clock across rejection paths so a stuck player cannot flood the log.
    if (DiagnosticLog::Due(blockedLog_)) {
        auto contact=terrain_.find(std::make_pair(x,y));
        float nz=contact==terrain_.end() ? 0 : contact->second.normal[2];
        int walkable=contact==terrain_.end() ? -1 : int(contact->second.walkable);
        // A report from a custom-map passage can distinguish local terrain contact from a model/pathing blocker.
        wc3::Trace("movement blocked reason=%s from=%.1f,%.1f sample=%.1f,%.1f reachable=%.1f terrain=%.1f cliff=%d/%d hullRadius=%.1f normalZ=%.3f walkable=%d",
            reason,oldX,oldY,x,y,bottom,terrain,
            wc3::GetTerrainCliffLevel(&oldX,&oldY),wc3::GetTerrainCliffLevel(&x,&y),radius_,nz,walkable);
    }
}
MovementSurface WarcraftCollision::Terrain(float x,float y,const MovementPhysics& movement) const {
    auto key=std::make_pair(x,y);auto found=terrain_.find(key);
    if(found!=terrain_.end())return found->second;
    MovementSurface surface;surface.height=wc3::TerrainGround(x,y);
    // One-sided local derivatives preserve flat ledge lips. A centered derivative would incorrectly
    // call the top of a step steep merely because its opposite side drops away.
    constexpr float probe=8;
    float derivatives[2],continuation[2];
    for(int axis=0;axis<2;++axis) {
        float dx=axis ? 0 : probe,dy=axis ? probe : 0;
        float before=(surface.height-wc3::TerrainGround(x-dx,y-dy))/probe;
        float after=(wc3::TerrainGround(x+dx,y+dy)-surface.height)/probe;
        derivatives[axis]=std::abs(before)<std::abs(after) ? before : after;
        // A tiny first sliver of a continuous steep face cannot supply a new takeoff.
        // Truly flat lips keep the one-sided derivative, while a shallow rim stays shallow on both sides.
        continuation[axis]=before*after>0 ? std::max(std::abs(before),std::abs(after)) : std::abs(derivatives[axis]);
    }
    float slope=std::hypot(derivatives[0],derivatives[1]),length=std::sqrt(1+slope*slope);
    surface.normal[0]=-derivatives[0]/length;surface.normal[1]=-derivatives[1]/length;surface.normal[2]=1/length;
    // A near-flat blocked rim is still ground. Only a rise exceeding the configured step
    // across a native terrain cell denotes a cliff face; tiny heightfield noise cannot close a passage.
    bool cliffFace=std::hypot(continuation[0],continuation[1])*128>movement.StepHeight()+.01f &&
        wc3::IsTerrainPathable(&x,&y,1) && CliffCell(x,y);
    surface.walkable=std::isfinite(slope) && !cliffFace &&
        surface.normal[2]>=std::cos(movement.MaxWalkSlope()*.01745329252f);
    surface.ramp=surface.walkable && slope>.001f;
    terrain_.emplace(key,surface);return surface;
}
MovementSurface WarcraftCollision::Floor(float x, float y, const MovementPhysics& movement, const std::vector<Bounds3>& obstacles) const {
    MovementSurface surface=Terrain(x,y,movement);float deck=0;
    float reachable = ReachableHeight(movement);
    auto support=[&](float height) {
        if(std::isfinite(height) && height<=reachable+.01f && height>=surface.height-.01f) {
            surface=MovementSurface{};surface.height=height;
        }
    };
    if (wc3::WalkableSurface(x, y, deck)) {
        support(deck);
    } else {
        // GetLocationZ also supplies native shallow-water support. Retain it when no overhead deck
        // is present, rather than lowering a Warcraft walker onto the submerged terrain mesh.
        if(surface.walkable) {
            float native=wc3::Ground(x,y);
            if(std::isfinite(native) && native<=reachable+.01f)surface.height=std::max(surface.height,native);
        }
    }
    // Use the same footprint as the side sweep: landing with a foot over a lip must catch its top
    // before the center crosses. Otherwise gravity sinks the feet below it and the next step hits its side.
    // Only already-reachable terrain can support us; a tall cliff remains a wall during ascent.
    for(int point=0;point<8;++point) {
        float angle=point*.78539816339f;
        float px=x+std::cos(angle)*radius_,py=y+std::sin(angle)*radius_;
        auto terrain=Terrain(px,py,movement);
        if(terrain.walkable && std::isfinite(terrain.height) && terrain.height<=reachable+.01f && terrain.height>surface.height)
            surface=terrain;
        // A bridge edge under the leading/trailing foot supports a landing just like a model lip.
        if(wc3::WalkableSurface(px,py,deck))support(deck);
    }
    // A low solid prop supports the entire foot too; support ends only after its trailing edge clears.
    for(size_t index=0;index<obstacles.size();++index) {
        const auto& box=obstacles[index];
        if(!Overlaps(box,x,y,radius_))continue;
        if(const auto* mesh=obstacles_.Geometry(index)) {
            float height=0,normal[3]={};
            if(mesh->Floor(x,y,radius_,reachable,.0001f,height,normal) && height>=surface.height-.01f) {
                surface.height=height;
                for(int axis=0;axis<3;++axis)surface.normal[axis]=normal[axis];
                // Steep prop roofs behave like steep terrain: contact slides downhill instead of falling through.
                surface.walkable=normal[2]>=std::cos(movement.MaxWalkSlope()*.01745329252f);
                surface.ramp=surface.walkable && normal[2]<.999999f;
            }
        } else if(box.maximum[2]<=reachable+.01f)support(box.maximum[2]);
    }
    return surface;
}
void WarcraftCollision::Ceiling(float x,float y,MovementPhysics& movement,const std::vector<Bounds3>& obstacles) const {
    // Sweep the head even with zero horizontal motion. Model undersides and native decks are two-sided.
    for(size_t index=0;index<obstacles.size();++index) {
        const auto& box=obstacles[index];
        if(!Overlaps(box,x,y,radius_))continue;
        if(const auto* mesh=obstacles_.Geometry(index)) {
            float height=0;
            if(mesh->Ceiling(x,y,radius_,movement.PreviousFeetZ(),height))movement.ClipHead(height-.01f);
        } else if(box.minimum[2]>movement.PreviousFeetZ()+.01f)movement.ClipHead(box.minimum[2]-.01f);
    }
    for(int point=0;point<9;++point) {
        float angle=(point-1)*.78539816339f,deck=0;
        float px=x+(point ? std::cos(angle)*radius_ : 0),py=y+(point ? std::sin(angle)*radius_ : 0);
        if(wc3::WalkableSurface(px,py,deck) && deck>movement.PreviousFeetZ()+.01f)movement.ClipHead(deck-.01f);
    }
}
bool WarcraftCollision::Clear(float x, float y, float oldX, float oldY, const MovementPhysics& movement,
    const std::vector<Bounds3>& obstacles,float* normal) const {
    float bottom = ReachableHeight(movement),feet=movement.FeetZ();
    auto support=Floor(x,y,movement,obstacles);
    if(movement.Grounded() && support.walkable && support.height<=bottom+.01f)feet=std::max(feet,support.height);
    float top=feet+movement.BodyHeight();
    for(size_t index=0;index<obstacles.size();++index) {
        const auto& box=obstacles[index];
        if(const auto* mesh=obstacles_.Geometry(index)) {
            if(!Overlaps(box,x,y,radius_))continue;
            float contact[2]={};
            if(!mesh->Contact(x,y,radius_,std::max(feet,bottom),top,contact))continue;
            // Existing mesh overlap can recover along its actual outward face, including arch interiors.
            // Newly raised headroom is never treated as an old overlap.
            bool existing=mesh->Contact(oldX,oldY,radius_,movement.FeetZ(),movement.FeetZ()+movement.BodyHeight());
            if(existing && (x-oldX)*contact[0]+(y-oldY)*contact[1]>.001f)continue;
            if(normal){normal[0]=contact[0];normal[1]=contact[1];}
            Blocked("solid-mesh",x,y,oldX,oldY,bottom,box.maximum[2]);return false;
        }
        // Height-aware solid volumes prevent flying through buildings/trees, while low props can be cleared.
        if (Overlaps(box, x, y, radius_) && box.maximum[2] > bottom + .01f && box.minimum[2] < top) {
            // Recover from existing overlap only while approaching an exit; never deepen it or enter a new solid.
            bool existing=Overlaps(box,oldX,oldY,radius_) && box.maximum[2]>movement.FeetZ()+.01f &&
                box.minimum[2]<movement.FeetZ()+movement.BodyHeight()-.01f;
            // A step which would newly push the head into an overhead box is not existing overlap.
            if (existing && ExitDistance(box,x,y,radius_)<ExitDistance(box,oldX,oldY,radius_)-.001f) continue;
            if(normal)CylinderHull::Normal(box,x,y,normal);
            Blocked("solid-volume",x,y,oldX,oldY,bottom,box.maximum[2]);return false;
        }
    }
    float terrainBottom=bottom, centerTerrain=wc3::TerrainGround(x,y);
    bool modelPath=false;
    // At takeoff/landing the footprint may meet the uphill side before its center clears it. Keep
    // ordinary step tolerance near ground so a gentle slope does not erase hop momentum.
    if(!movement.Grounded() && std::abs(bottom-centerTerrain)<=movement.StepHeight())
        terrainBottom+=movement.StepHeight();
    for (int point = 0; point < 9; ++point) {
        float angle = (point - 1) * .78539816339f;
        float px = x + (point ? std::cos(angle) * radius_ : 0);
        float py = y + (point ? std::sin(angle) * radius_ : 0);
        auto sample=Terrain(px,py,movement);
        float terrain = sample.height, deck = 0;
        if (!std::isfinite(terrain)) { Blocked("invalid-terrain",px,py,oldX,oldY,bottom,terrain);return false; }
        // After leaving a ledge the trailing footprint still overlaps its lip: permit retreat from
        // existing terrain overlap, but never enter a higher surface or lift the center through it.
        float oldSample = wc3::TerrainGround(oldX + px - x, oldY + py - y);
        // Only locally walkable slopes receive step tolerance. A blocked interpolated cliff face
        // stays a side wall until the actual airborne feet clear it, without probing a distant terrace.
        // Flat ledge tops must obey the same airborne reach as landing. Otherwise ramp tolerance
        // could admit a short jump below an unreachable lip and strand it inside the raised terrain.
        float walkLimit=movement.Grounded() || sample.ramp ? terrainBottom : bottom;
        float limit=sample.walkable ? walkLimit : std::max(movement.FeetZ(),movement.PreviousFeetZ());
        if (terrain > limit + .01f && (!point || terrain > oldSample + .01f)) {
            if(normal) {
                float length=std::hypot(sample.normal[0],sample.normal[1]);
                if(length>.001f) {normal[0]=sample.normal[0]/length;normal[1]=sample.normal[1]/length;}
            }
            Blocked("uphill-terrain",px,py,oldX,oldY,bottom,terrain);return false;
        }
        if(wc3::WalkableSurface(px,py,deck) && deck>bottom+.01f && deck<top-.01f) {
            Blocked("deck-side",px,py,oldX,oldY,bottom,deck);return false;
        }
        if (!wc3::IsTerrainPathable(&px, &py, 1)) continue;
        // A deck is also overhead clearance: its 2D pathing must not close a genuinely tall passage below it.
        if (wc3::WalkableSurface(px, py, deck) && (deck <= bottom + .01f || deck>=top-.01f)) continue;
        bool knownObject = false;
        // The full body already cleared every known solid. Do not enlarge one to its native path cell
        // when only an empty corner of that cell overlaps the player's circular foot.
        for(size_t index=0;index<obstacles.size();++index) {
            const auto& box=obstacles[index];
            // All occupied surfaces already cleared the entire body. A native cell inside an
            // enclosing arch/column box must not fill that verified empty opening back in.
            if(obstacles_.Geometry(index) && ModelPathCell(box,px,py)) {
                knownObject=true;modelPath=true;continue;
            }
            if(!Overlaps(box,x,y,radius_) && ModelPathCell(box,px,py)) {
                knownObject=true;modelPath=true;continue;
            }
            if (Overlaps(box, px, py, radius_) &&
            (box.maximum[2] <= bottom + .01f || box.minimum[2]>=top-.01f ||
            // Native footprints can exceed the mesh: allow the trailing foot samples to clear a nearby
            // known solid while retreating. The solid-volume check above still rejects entering its mesh.
            (Overlaps(box,oldX,oldY,radius_*2) && ExitDistance(box,x,y,radius_)<ExitDistance(box,oldX,oldY,radius_)-.001f))) knownObject = true;
        }
        if (knownObject) continue;
        // Native cliff cells block walkers even above an edge. Recognize their cliff-level discontinuity,
        // keeping flat unknown blockers closed rather than disabling pathing for every jump.
        if (CliffCell(px,py) || wc3::GetTerrainCliffLevel(&oldX,&oldY)>wc3::GetTerrainCliffLevel(&px,&py)) continue;
        Blocked("native-pathing",px,py,oldX,oldY,bottom,terrain);return false;
    }
    if(modelPath)modelPathing_=true;
    return true;
}
bool WarcraftCollision::Safe(float x,float y,float z,const MovementPhysics& movement,const std::vector<Bounds3>& obstacles) const {
    if(!std::isfinite(z))return false;
    for(int point=0;point<9;++point) {
        float angle=(point-1)*.78539816339f;
        float px=x+(point ? std::cos(angle)*radius_ : 0),py=y+(point ? std::sin(angle)*radius_ : 0),deck=0;
        float ground=wc3::TerrainGround(px,py);
        if(!std::isfinite(ground) || ground>z+.01f)return false;
        // Recovery checkpoints use natively walkable space, not the permissive cliff/model path exceptions.
        if(wc3::IsTerrainPathable(&px,&py,1) &&
            !(wc3::WalkableSurface(px,py,deck) && deck<=z+.01f && deck>=z-movement.StepHeight()-.01f))return false;
        if(wc3::WalkableSurface(px,py,deck) && deck>z+.01f && deck<z+movement.BodyHeight()-.01f)return false;
    }
    for(size_t index=0;index<obstacles.size();++index) {
        const auto& box=obstacles[index];
        if(!Overlaps(box,x,y,radius_))continue;
        if(const auto* mesh=obstacles_.Geometry(index)) {
            if(mesh->Contact(x,y,radius_,z,z+movement.BodyHeight()))return false;
        } else if(box.maximum[2]>z+.01f && box.minimum[2]<z+movement.BodyHeight()-.01f)return false;
    }
    return true;
}
void WarcraftCollision::Recover(wc3::Handle unit,float x,float y,MovementPhysics& movement,const std::vector<Bounds3>& obstacles) {
    auto terrain=Terrain(x,y,movement);
    float deck=0;
    // Remember actual support provenance. Someone who walked underneath a bridge is never
    // lifted onto it; a rider who lost its still-present deck may recover onto that same deck.
    bool deckHere=wc3::WalkableSurface(x,y,deck);
    bool lostDeck=ridingDeck_ && deckHere && movement.FeetZ()<deck-.01f && movement.PreviousFeetZ()<deck-.01f;
    bool buried=!std::isfinite(movement.FeetZ()) || (std::isfinite(terrain.height) &&
        movement.FeetZ()<terrain.height-.01f && movement.PreviousFeetZ()<terrain.height-.01f);
    bool manual=recoveryRequested_;recoveryRequested_=false;
    // Only a proven invalid position is repaired automatically; ordinary drops and pits keep falling normally.
    if(movement.RecoverInvalidPosition() && lostDeck && Safe(x,y,deck,movement,obstacles)) {
        wc3::Trace("movement recovery reason=lost-deck x=%.1f y=%.1f fromZ=%.1f toZ=%.1f",x,y,movement.FeetZ(),deck);
        movement.RecoverFloor(deck);
    } else if(movement.RecoverInvalidPosition() && buried && terrain.walkable && Safe(x,y,terrain.height,movement,obstacles)) {
        wc3::Trace("movement recovery reason=below-terrain x=%.1f y=%.1f fromZ=%.1f toZ=%.1f",x,y,movement.FeetZ(),terrain.height);
        movement.RecoverFloor(terrain.height);
    } else if(manual || (movement.RecoverInvalidPosition() && (buried || lostDeck))) {
        MovementRecovery::Point point;
        // From valid lower ground, Home can return to a recent upper bank. On ordinary open
        // ground it cannot become a horizontal teleport. Invalid/wedged positions may use any safe height.
        float minimumRise=Safe(x,y,movement.FeetZ(),movement,obstacles) ?
            (movement.Grounded() ? movement.StepHeight()+.01f : .01f) : -std::numeric_limits<float>::infinity();
        auto validate=[&](const MovementRecovery::Point& candidate) {
            auto probe=movement;probe.RecoverFloor(candidate.z);
            auto floor=Floor(candidate.x,candidate.y,probe,obstacles);
            return floor.walkable && std::abs(floor.height-candidate.z)<.01f &&
                Safe(candidate.x,candidate.y,candidate.z,movement,obstacles);
        };
        if(recovery_.Find(x,y,movement.FeetZ(),movement.RecoveryDistance(),minimumRise,validate,point)) {
            wc3::Trace("movement recovery reason=%s from=%.1f,%.1f,%.1f to=%.1f,%.1f,%.1f",manual ? "manual" : "invalid",x,y,movement.FeetZ(),point.x,point.y,point.z);
            wc3::SetUnitX(unit,&point.x);wc3::SetUnitY(unit,&point.y);movement.RecoverFloor(point.z);
            x=point.x;y=point.y;
        } else if(manual)wc3::Trace("movement recovery unavailable: no revalidated nearby checkpoint");
    }
    auto surface=Floor(x,y,movement,obstacles);
    if(movement.Grounded() && surface.walkable && std::abs(movement.FeetZ()-surface.height)<.01f &&
        Safe(x,y,movement.FeetZ(),movement,obstacles))
        recovery_.Record(x,y,movement.FeetZ(),radius_*2);
    if(movement.Grounded()) {
        deckHere=wc3::WalkableSurface(x,y,deck);
        ridingDeck_=deckHere && deck<=movement.FeetZ()+.01f && deck>=movement.FeetZ()-movement.StepHeight()-.01f;
    }
    lastX_=x;lastY_=y;havePosition_=true;
}
void WarcraftCollision::Move(wc3::Handle unit, MovementPhysics& movement, float dt) {
    dt = std::clamp(dt, 0.f, .05f);
    terrain_.clear();
    modelPathing_=false;
    float x = wc3::Real(wc3::GetUnitX(unit)), y = wc3::Real(wc3::GetUnitY(unit));
    // Scripted relocations, view changes and actor switches must never reuse a checkpoint from another place.
    if(actor_!=unit || (havePosition_ && std::hypot(x-lastX_,y-lastY_)>radius_*2)){recovery_.Reset();ridingDeck_=false;}
    actor_=unit;
    float originalX = x, originalY = y;
    auto body=movement.NativeUnitCollision() ? UnitCollisionProfile::Read(unit,base_) : UnitCollisionProfile{};
    // FPS may disable the actor's native pathing mask. Its known footprint remains valid,
    // unlike non-colliding helper obstacles, and must not suddenly inflate in a narrow corridor.
    radius_=body.known && body.radius>0 ? std::min(body.radius,movement.HullRadius()) : movement.HullRadius();
    float dx = movement.VelocityX() * dt, dy = movement.VelocityY() * dt;
    auto obstacles = obstacles_.Snapshot(unit, x, y, std::max(600.f,movement.RecoveryDistance()+radius_) + std::hypot(dx, dy),movement.NativeUnitCollision(),movement.ObjectMeshCollision());
    auto bounds = wc3::GetWorldBounds();
    float minX = wc3::Real(wc3::GetRectMinX(bounds)) + radius_, maxX = wc3::Real(wc3::GetRectMaxX(bounds)) - radius_;
    float minY = wc3::Real(wc3::GetRectMinY(bounds)) + radius_, maxY = wc3::Real(wc3::GetRectMaxY(bounds)) - radius_;
    wc3::RemoveRect(bounds);
    Ceiling(x,y,movement,obstacles);
    if (std::hypot(dx,dy)<.001f) {
        // Standing still needs floor resolution, but no collision rejection or blocked-movement telemetry.
        movement.ResolveSurface(Floor(x,y,movement,obstacles));Recover(unit,x,y,movement,obstacles);return;
    }
    int steps = std::max(1, int(std::ceil(std::hypot(dx, dy) / 8)));
    // Eight-unit swept steps retain thin-wall collision at maximum bunnyhop speed.
    for (int step = 0; step < steps; ++step) {
        float nx = x + dx / steps, ny = y + dy / steps;
        if (nx < minX || nx > maxX) { Blocked("world-boundary",nx,ny,x,y,movement.FeetZ(),0);nx = x; movement.BlockX(); dx = 0; }
        if (ny < minY || ny > maxY) { Blocked("world-boundary",nx,ny,x,y,movement.FeetZ(),0);ny = y; movement.BlockY(); dy = 0; }
        float normal[2]={};
        if (Clear(nx, ny, x, y, movement, obstacles,normal)) { x = nx; y = ny; }
        else {
            float into=(nx-x)*normal[0]+(ny-y)*normal[1];
            float sx=nx-into*normal[0],sy=ny-into*normal[1];
            // Try the actual contact tangent before axis fallback; curved corners must not eat forward motion.
            if(into<-.0001f && std::hypot(sx-x,sy-y)>.0001f && Clear(sx,sy,x,y,movement,obstacles)) {
                x=sx;y=sy;movement.SlideHorizontal(normal[0],normal[1]);
                dx=movement.VelocityX()*dt;dy=movement.VelocityY()*dt;
            } else {
                if (Clear(nx, y, x, y, movement, obstacles)) x = nx; else { movement.BlockX(); dx = 0; }
                if (Clear(x, ny, x, y, movement, obstacles)) y = ny; else { movement.BlockY(); dy = 0; }
            }
        }
        // Follow support after each accepted ground substep. Fast uphill sweeps cannot reuse an outdated frame-start floor.
        // Airborne feet remain controlled by the vertical integrator.
        if(movement.Grounded())movement.ResolveSurface(Floor(x,y,movement,obstacles));
    }
    float floor = Floor(x, y, movement, obstacles).height;
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
        bool cappedHull=body.known && (body.radius>radius_+.01f || !body.colliding);
        if (!movement.Grounded() || movement.FeetZ() - floor > movement.StepHeight() || raised || nearCliff || nearSolid || cappedHull || modelPathing_) {
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
    Ceiling(x,y,movement,obstacles);
    movement.ResolveSurface(Floor(x, y,movement,obstacles));
    Recover(unit,x,y,movement,obstacles);
}
