#include "../src/platform/WarcraftCollision.hpp"
#include "../src/platform/DiagnosticLog.hpp"
#include "../src/platform/UnitCollisionProfile.hpp"
#include "MovementStrafeInput.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace {
float unitX=0,unitY=0;
bool bridgeAlive=true,edge=false,nativeBlocked=false,cliff=false,invalidDeck=false;
bool drop=false, prop=false, wall=false, slope=false, water=false;
bool tree=false,canopyBounds=false;
bool wideCliff=false;
bool landingLedge=false;
float ledgeYaw=0,cliffRim=144;
float actorRadius=0;
bool nativeCliffRim=false;
float slopeGradient=.3f;
bool narrowPassage=false;
int sideContact=0,sideRelocations=0;bool sideUnit=false;
wc3::Bits Bits(float value) { wc3::Bits bits;std::memcpy(&bits,&value,4);return bits; }
wc3::Bits __cdecl X(wc3::Handle) {return Bits(unitX);}
wc3::Bits __cdecl Y(wc3::Handle) {return Bits(unitY);}
BOOL __cdecl Blocked(float* x,float* y,int) {
    if(landingLedge)return FALSE;
    if(sideContact)return FALSE; // Unit collision/side volumes are independent of the terrain pathing grid.
    // Stock tree pathing retains its trunk footprint while the canopy above remains empty space.
    if(tree) return *x>=88 && *x<=152 && std::abs(*y)<=32;
    if(slope) return FALSE;
    if(narrowPassage)return std::abs(*y)>20;
    if (drop) return *x >= 8 && *x <= (wideCliff ? cliffRim : 56);
    if (prop || wall) return *x >= 60 && *x <= 80;
    return *x>=32;
}
void __cdecl SetX(wc3::Handle,float* x) { unitX=*x; }
void __cdecl SetY(wc3::Handle,float* y) { unitY=*y; }
wc3::Handle __cdecl World() { return 1; }
wc3::Bits __cdecl Minimum(wc3::Handle) { return Bits(-4096); }
wc3::Bits __cdecl Maximum(wc3::Handle) { return Bits(4096); }
void __cdecl Remove(wc3::Handle) {}
int __cdecl Level(float* x,float*) { return (drop || cliff) && *x>=32 ? (drop ? 1 : 3) : 2; }
void __cdecl Position(wc3::Handle,float* x,float* y) {
    // A clear center beside a cliff can still trigger native nearest-cell placement for its wider hull.
    if(nativeCliffRim && *x>0 && *x<8){unitX=*x+80;unitY=*y;return;}
    // Native placement may briefly publish a sideways snap even if the caller later restores logical XY.
    if(sideContact && (!sideUnit || std::abs(*x)<96) && *y*sideContact>8) {
        unitX=*x;unitY=*y+sideContact*80;++sideRelocations;return;
    }
    // Native Warcraft may relocate a blocked unit; collision must discard that search result.
    unitX=*x+(nativeBlocked && *x>0 && (!tree || *x>=64) ? 80 : 0);unitY=*y;
}
void Require(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
float Walk(bool alive,bool atEdge=false,bool obstruction=false,bool highCliff=false,bool invalid=false) {
    unitX=unitY=0;bridgeAlive=alive;edge=atEdge;nativeBlocked=obstruction;cliff=highCliff;invalidDeck=invalid;
    MovementPhysics movement;movement.Reset(100);WarcraftCollision collision;MoveInput input;input.forward=1;
    for(int frame=0;frame<60;++frame){movement.Step(input,1.f/60,250);collision.Move(1,movement,1.f/60);}
    Require(std::abs(movement.FeetZ()-wc3::Ground(unitX,unitY))<.01f,"feet follow native bridge deck height");
    return unitX;
}
}
namespace wc3 {
void Trace(const char*,...) {}
int (__cdecl* GetTerrainCliffLevel)(float*,float*)=Level;
Bits (__cdecl* GetUnitX)(Handle)=X;
Bits (__cdecl* GetUnitY)(Handle)=Y;
BOOL (__cdecl* IsTerrainPathable)(float*,float*,int)=Blocked;
void (__cdecl* SetUnitPosition)(Handle,float*,float*)=Position;
void (__cdecl* SetUnitX)(Handle,float*)=SetX;
void (__cdecl* SetUnitY)(Handle,float*)=SetY;
Handle (__cdecl* GetWorldBounds)()=World;
Bits (__cdecl* GetRectMinX)(Handle)=Minimum;
Bits (__cdecl* GetRectMinY)(Handle)=Minimum;
Bits (__cdecl* GetRectMaxX)(Handle)=Maximum;
Bits (__cdecl* GetRectMaxY)(Handle)=Maximum;
void (__cdecl* RemoveRect)(Handle)=Remove;
float Real(Bits bits){float f;std::memcpy(&f,&bits,4);return f;}
float TerrainGround(float x,float y){return landingLedge ? (x*std::cos(ledgeYaw)+y*std::sin(ledgeYaw)>=32 ? 140.f : 100.f) : slope ? 100.f+x*slopeGradient : drop && x>=32 ? 0.f : cliff && x>=32 ? 180.f : 100.f;}
float Ground(float x,float y){return TerrainGround(x,y)+(water ? 22.f : 0.f);}
bool WalkableSurface(float,float y,float& height){
    if(!bridgeAlive || (edge && std::abs(y)>16))return false;
    height=invalidDeck ? std::numeric_limits<float>::quiet_NaN() : 100.f;return !invalidDeck;
}
}
bool DiagnosticLog::Due(DWORD&,bool) { return false; }
// The synthetic world replaces only native discovery; actual swept collision/physics runs unchanged.
std::vector<Bounds3> MovementObstacles::Snapshot(wc3::Handle,float,float,float,bool) {
    if(sideContact) {
        float reach=sideUnit ? 64.f : 4096.f;
        return {{{-reach,sideContact>0 ? 48.f : -64.f,100},{reach,sideContact>0 ? 64.f : -48.f,300}}};
    }
    if (tree) return canopyBounds ? std::vector<Bounds3>{{{-93,-231,100},{334,193,413}}} :
        std::vector<Bounds3>{{{101,-21,100},{138,22,384}}};
    if (prop || wall) return {{{60,-50,100},{80,50,wall ? 300.f : 140.f}}};
    return {};
}
UnitCollisionProfile UnitCollisionProfile::Read(wc3::Handle,uintptr_t) {
    if(actorRadius>0)return {true,true,actorRadius};
    return narrowPassage ? UnitCollisionProfile{true,true,16} : UnitCollisionProfile{};
}
int main(){
    Require(Walk(true)>300,"living deck crosses blocked terrain beneath the bridge");
    Require(Walk(false)<32,"destroyed bridge supplies no walkable geometry");
    Require(Walk(true,true)<32,"entire player footprint must fit on the deck");
    Require(Walk(true,false,true)<.01f,"native building collision cannot be bypassed by a bridge");
    Require(Walk(true,false,false,true)<32,"bridge support does not permit a vertical cliff step");
    Require(Walk(true,false,false,false,true)<32,"invalid surface samples fail closed");
    unitX=unitY=0; bridgeAlive=false; edge=nativeBlocked=cliff=invalidDeck=false; drop=true;
    MovementPhysics falling; falling.Reset(100); WarcraftCollision collision; MoveInput forward; forward.forward=1;
    bool fell=false;
    for (int frame=0;frame<100;++frame) {
        falling.Step(forward,1.f/60,250); collision.Move(1,falling,1.f/60);
        if (!falling.Grounded() && falling.FeetZ()>0) fell=true;
    }
    Require(unitX>300 && fell && falling.Grounded() && falling.FeetZ()==0,"walking off a native blocked cliff must fall and land");
    // A Warcraft cliff's blocked rim can extend a full terrain cell onto the lower bank.
    wideCliff=true;unitX=unitY=0;falling.Reset(100);
    for(int frame=0;frame<100;++frame){falling.Step(forward,1.f/60,250);collision.Move(1,falling,1.f/60);}
    Require(unitX>300 && falling.Grounded() && falling.FeetZ()==0,"full-cell cliff pathing must not trap a player after landing");
    // Native cliff-level lookup changes before the far end of the blocked lower-bank cell.
    cliffRim=272;unitX=unitY=0;falling.Reset(100);
    for(int frame=0;frame<100;++frame){falling.Step(forward,1.f/60,250);collision.Move(1,falling,1.f/60);}
    Require(unitX>300 && falling.Grounded() && falling.FeetZ()==0,"the outer lower-bank pathing cell must not trap a landed footprint");
    nativeCliffRim=true;unitX=unitY=0;falling.Reset(100);
    for(int frame=0;frame<100;++frame){falling.Step(forward,1.f/60,250);collision.Move(1,falling,1.f/60);}
    Require(unitX>300,"a clear cliff-side center must not invoke native hull relocation");
    nativeCliffRim=false;
    wideCliff=false;
    drop=false;prop=true;unitX=-50;unitY=0;
    MovementPhysics jumper;jumper.Reset(100);MoveInput jumping=forward;jumping.jump=true;
    // Approach at running speed so the jump has enough range to clear the prop's full footprint.
    for(int frame=0;frame<60;++frame) jumper.Step(forward,1.f/60,250);
    for(int frame=0;frame<70;++frame){jumper.Step(jumping,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(unitX>80,"jump clears a low prop despite its 2D pathing footprint");
    prop=false;wall=true;unitX=unitY=0;jumper.Reset(100);
    for(int frame=0;frame<70;++frame){jumper.Step(jumping,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(unitX<36,"airborne player cannot pass through a tall obstacle");
    wall=false;unitX=unitY=0;jumper.Reset(100);
    for(int frame=0;frame<70;++frame){jumper.Step(jumping,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(unitX<32,"airborne motion must not disable unknown flat pathing blockers");
    slope=true;unitX=unitY=0;jumper.Reset(100);float peak=0;
    for(int frame=0;frame<240;++frame) {
        MoveInput uphill = frame >= 60 ? MovementStrafeInput(jumper, float(frame-60)/60) : forward;
        jumper.Step(uphill,1.f/60,250);collision.Move(1,jumper,1.f/60);
        peak=std::max(peak,jumper.Speed());
    }
    std::printf("Uphill hop distance=%.1f peak=%.1f feet=%.1f\n",unitX,peak,jumper.FeetZ());
    Require(unitX>1000 && peak>450,"landing on a gentle uphill slope preserves accelerating hop momentum");
    // At the fastest supported setting, one frame travels farther than a thin wall's full width.
    // Actual swept collision must stop the player instead of sampling only the destination.
    MovementSettings fast; fast.maxBunnySpeed = 2000; fast.jumpBoostPercent = 100;
    jumper.Configure(fast); jumper.Reset(100);
    for(int frame=0;frame<360;++frame){jumper.Step(jumping,1.f/60,250);jumper.ResolveFloor(100);}
    Require(jumper.Speed()>2900,"high-speed collision trial must approach the supported maximum");
    slope=false;wall=true;unitX=unitY=0;
    collision.Move(1,jumper,.05f);
    Require(unitX<36,"swept collision must reject a thin wall at maximum bunnyhop speed");
    wall=false;slope=true;jumper.Configure(MovementSettings{});
    // Water support from GetLocationZ is independent of a native walkable bridge deck.
    water=true;unitX=unitY=0;jumper.Reset(122);jumping.jump=false;
    for(int frame=0;frame<60;++frame){jumper.Step(jumping,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(jumper.Grounded() && std::abs(jumper.FeetZ()-wc3::Ground(unitX,unitY))<.01f,"native shallow-water support must not become a false cliff drop");
    water=slope=false;tree=true;
    auto treeWalk=[&](bool broad,float y) {
        canopyBounds=broad;unitX=0;unitY=y;jumper.Reset(100);
        for(int frame=0;frame<60;++frame){jumper.Step(forward,1.f/60,250);collision.Move(1,jumper,1.f/60);}
        return unitX;
    };
    // Reproduce a player already inside the old visual box: even retreat was blocked by the canopy.
    Require(std::abs(treeWalk(true,96))<.01f,"canopy-sized bounds must reproduce the false tree blocker");
    Require(treeWalk(false,96)>300,"decoded trunk bounds permit a corridor beside the tree canopy");
    Require(treeWalk(false,0)<80,"walking directly into the tree must remain blocked");
    // Native spawning/RTS relocation can leave the player inside a trunk: retreat must work.
    nativeBlocked=true;unitX=90;unitY=0;jumper.Reset(100);MoveInput retreat=forward;retreat.forward=-1;
    for(int frame=0;frame<60;++frame){jumper.Step(retreat,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(unitX<0,"existing trunk overlap permits movement toward the nearest exit");
    nativeBlocked=false;
    unitX=90;unitY=0;jumper.Reset(100);
    for(int frame=0;frame<20;++frame){jumper.Step(forward,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(unitX==90,"existing overlap must not permit entering deeper into a trunk");
    tree=false;
    // Warcraft's 16-unit actor fits a narrow walkable corridor that the legacy 24-unit footprint blocks.
    narrowPassage=true;unitX=unitY=0;jumper.Reset(100);jumper.Configure(MovementSettings{});
    for(int frame=0;frame<60;++frame){jumper.Step(forward,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(unitX>300,"live native actor footprint fits the narrow native passage");
    auto legacy=MovementSettings{};legacy.useNativeUnitCollision=false;jumper.Configure(legacy);unitX=unitY=0;jumper.Reset(100);
    for(int frame=0;frame<60;++frame){jumper.Step(forward,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(unitX==0,"legacy opt-out retains the larger visual footprint");narrowPassage=false;
    // At 20 FPS a frame contains multiple sweep steps. Resolve support between them on uphill/downhill travel.
    slope=true;slopeGradient=.8f;jumper.Configure(MovementSettings{});unitX=unitY=0;jumper.Reset(100);
    for(int frame=0;frame<20;++frame){jumper.Step(forward,.05f,250);collision.Move(1,jumper,.05f);}
    Require(unitX>300 && jumper.Grounded(),"fast grounded sweeps traverse a steep native ramp uphill");
    retreat=forward;retreat.forward=-1;
    for(int frame=0;frame<40;++frame){jumper.Step(retreat,.05f,250);collision.Move(1,jumper,.05f);}
    Require(unitX<0 && jumper.Grounded(),"fast grounded sweeps traverse the ramp downhill");
    // Left/right side contacts retain forward sliding without ever publishing a native relocation before rollback.
    slope=false;bridgeAlive=false;
    for(bool unit:{false,true})for(int side:{-1,1}) {
        sideUnit=unit;sideContact=side;sideRelocations=0;unitX=unitY=0;jumper.Reset(100);
        MoveInput diagonal=forward;diagonal.right=float(-side);
        for(int frame=0;frame<60;++frame) {
            float x=unitX,y=unitY;jumper.Step(diagonal,1.f/60,250);
            float allowed=jumper.Speed()/60+.01f; // Wall sliding can retain more tangent speed than an open diagonal.
            collision.Move(1,jumper,1.f/60);
            Require(std::hypot(unitX-x,unitY-y)<=allowed,"side contact cannot teleport beyond the integrated motion");
            if(!unit || std::abs(unitX)<64)Require(unitY*side<=24.01f,"side contact still blocks penetration");
        }
        Require(sideRelocations==0,"validated side movement must never invoke a snapping native placement");
        Require(unitX>200,"holding forward against a side obstacle must retain tangential sliding");
    }
    sideContact=0;
    // Land with only the leading foot on a ledge, then continue onto it without another jump.
    // Center-only support lets the feet sink below the lip and blocks the next forward step.
    landingLedge=true;
    for(float radius:{0.f,16.f,32.f})for(int fps:{20,60,144})for(int heading=0;heading<8;++heading) {
        actorRadius=radius; // Cover the legacy fallback and both common native actor footprints.
        ledgeYaw=heading*.78539816339f;
        float cx=std::cos(ledgeYaw),cy=std::sin(ledgeYaw),dt=1.f/fps;
        unitX=20*cx;unitY=20*cy;jumper.Reset(100);
        MoveInput vertical;vertical.jump=true;
        bool above=false;
        for(int frame=0;frame<fps*2;++frame) {
            jumper.Step(vertical,dt,250);collision.Move(1,jumper,dt);
            above=above || jumper.FeetZ()>140;
            vertical.jump=false;
            if(above && jumper.Grounded())break;
        }
        Require(above && jumper.Grounded() && jumper.FeetZ()==140,"a descending footprint lands on the ledge lip before its center crosses");
        MoveInput toward=forward;toward.yaw=heading*45.f;
        for(int frame=0;frame<fps;++frame){jumper.Step(toward,dt,250);collision.Move(1,jumper,dt);}
        Require(unitX*cx+unitY*cy>200 && jumper.Grounded(),"landing on the lip must allow continuing onto the upper surface");
        toward.forward=-1;bool fallingOff=false;
        for(int frame=0;frame<fps*3;++frame) {
            jumper.Step(toward,dt,250);collision.Move(1,jumper,dt);
            fallingOff=fallingOff || (!jumper.Grounded() && jumper.FeetZ()>100);
        }
        Require(unitX*cx+unitY*cy<0 && fallingOff && jumper.Grounded() && jumper.FeetZ()==100,"leaving the lip falls and lands without hovering or sticking");
    }
    // Model-backed low props share the same rim support rule as terrain, including a partial landing.
    actorRadius=0;landingLedge=false;prop=true;unitX=48;unitY=0;jumper.Reset(100);
    MoveInput vertical;vertical.jump=true;bool above=false;
    for(int frame=0;frame<120;++frame) {
        jumper.Step(vertical,1.f/60,250);collision.Move(1,jumper,1.f/60);
        above=above || jumper.FeetZ()>140;vertical.jump=false;
        if(above && jumper.Grounded())break;
    }
    Require(above && jumper.Grounded() && jumper.FeetZ()==140,"the edge of a solid prop must catch a descending foot");
    for(int frame=0;frame<90;++frame){jumper.Step(forward,1.f/60,250);collision.Move(1,jumper,1.f/60);}
    Require(unitX>200 && jumper.Grounded() && jumper.FeetZ()==100,"a partial prop landing permits traversal and a clean drop off its far edge");
    std::puts("Warcraft bridge/collision regressions passed.");
}
