#include "../src/platform/WarcraftCollision.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace {
float unitX=0,unitY=0;
bool bridgeAlive=true,edge=false,nativeBlocked=false,cliff=false,invalidDeck=false;
wc3::Bits Bits(float value) { wc3::Bits bits;std::memcpy(&bits,&value,4);return bits; }
wc3::Bits __cdecl X(wc3::Handle) {return Bits(unitX);}
wc3::Bits __cdecl Y(wc3::Handle) {return Bits(unitY);}
BOOL __cdecl Blocked(float* x,float*,int) { return *x>=32; }
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
Bits (__cdecl* GetUnitX)(Handle)=X;
Bits (__cdecl* GetUnitY)(Handle)=Y;
BOOL (__cdecl* IsTerrainPathable)(float*,float*,int)=Blocked;
void (__cdecl* SetUnitPosition)(Handle,float*,float*)=Position;
float Real(Bits bits){float f;std::memcpy(&f,&bits,4);return f;}
float Ground(float x,float){return cliff && x>=32 ? 180.f : 100.f;}
bool WalkableSurface(float,float y,float& height){
    if(!bridgeAlive || (edge && std::abs(y)>16))return false;
    height=invalidDeck ? std::numeric_limits<float>::quiet_NaN() : 100.f;return !invalidDeck;
}
}
int main(){
    Require(Walk(true)>300,"living deck crosses blocked terrain beneath the bridge");
    Require(Walk(false)<32,"destroyed bridge supplies no walkable geometry");
    Require(Walk(true,true)<32,"entire player footprint must fit on the deck");
    Require(Walk(true,false,true)<.01f,"native building collision cannot be bypassed by a bridge");
    Require(Walk(true,false,false,true)<32,"bridge support does not permit a vertical cliff step");
    Require(Walk(true,false,false,false,true)<32,"invalid surface samples fail closed");
    std::puts("Warcraft bridge/collision regressions passed.");
}
