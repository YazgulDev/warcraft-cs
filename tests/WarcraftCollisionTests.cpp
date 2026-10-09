#include "../src/platform/WarcraftCollision.hpp"
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
wc3::Bits Bits(float value) { wc3::Bits bits;std::memcpy(&bits,&value,4);return bits; }
wc3::Bits __cdecl X(wc3::Handle) {return Bits(unitX);}
wc3::Bits __cdecl Y(wc3::Handle) {return Bits(unitY);}
BOOL __cdecl Blocked(float* x,float*,int) {
    if(slope) return FALSE;
    if (drop) return *x >= 8 && *x <= 56;
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
    // Native Warcraft may relocate a blocked unit; collision must discard that search result.
    unitX=*x+(nativeBlocked && *x>0 ? 80 : 0);unitY=*y;
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
float TerrainGround(float x,float){return slope ? 100.f+x*.3f : drop && x>=32 ? 0.f : cliff && x>=32 ? 180.f : 100.f;}
float Ground(float x,float y){return TerrainGround(x,y)+(water ? 22.f : 0.f);}
bool WalkableSurface(float,float y,float& height){
    if(!bridgeAlive || (edge && std::abs(y)>16))return false;
    height=invalidDeck ? std::numeric_limits<float>::quiet_NaN() : 100.f;return !invalidDeck;
}
}
// The synthetic world replaces only native discovery; actual swept collision/physics runs unchanged.
std::vector<Bounds3> MovementObstacles::Snapshot(wc3::Handle,float,float,float) {
    if (prop || wall) return {{{60,-50,100},{80,50,wall ? 300.f : 140.f}}};
    return {};
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
    std::puts("Warcraft bridge/collision regressions passed.");
}
