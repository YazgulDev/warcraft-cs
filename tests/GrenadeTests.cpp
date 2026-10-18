#include "../src/combat/GrenadeProjectile.hpp"
#include "../src/combat/GrenadeThrow.hpp"
#include <cassert>

int main() {
    GrenadeThrow action;using E=GrenadeThrow::Event;
    assert(action.Step(0,true,true,1,.25f,.8f)==E::Pull);
    assert(action.Step(10,false,true,1,.25f,.8f)==E::None); // A held pin never starts a fuse.
    assert(action.Step(10.1,false,false,1,.25f,.8f)==E::Throw);
    assert(action.Step(10.2,false,false,1,.25f,.8f)==E::None);
    assert(action.Step(10.36,false,false,1,.25f,.8f)==E::Launch);
    assert(action.Step(10.4,false,false,1,.25f,.8f)==E::None);
    assert(action.Step(11,false,false,1,.25f,.8f)==E::Finished);
    assert(action.Step(12,true,false,1,.25f,.8f)==E::Pull);action.Reset();assert(!action.Active());
    GrenadeProjectile a,b;a.position=b.position={0,0,96};a.velocity=b.velocity={750,0,135};
    auto ground=[](float,float){return 0.f;};
    for(int i=1;i<=30;++i)a.Advance(i/30.,{},ground);
    for(int i=1;i<=144;++i)b.Advance(i/144.,{},ground);
    // Substep partition differences are bounded below one native unit, with no fuse dependence on render rate.
    for(int i=0;i<3;++i)assert(std::abs(a.position[i]-b.position[i])<1);
    GrenadeProjectile wall;wall.position={0,0,96};wall.velocity={750,0,0};Bounds3 box={{100,-50,0},{102,50,200}};
    wall.Advance(.3,{box},ground);assert(wall.position[0]<96 && wall.velocity[0]<0 && wall.bounced);
    wall.Advance(10,{box},ground);assert(std::abs(wall.age-wall.settings.fuse)<.001 && wall.position[2]>=4);
    auto saved=wall.position;wall.Advance(20,{box},ground);assert(saved==wall.position);
    assert(GrenadeProjectile::Falloff(0,350)==1 && GrenadeProjectile::Falloff(400,350)==0);
    GrenadePoint flash={100,0,0},eye={0,0,0};
    assert(GrenadeProjectile::Exposure(flash,eye,{1,0,0},1000)>GrenadeProjectile::Exposure(flash,eye,{-1,0,0},1000)*4);
}
