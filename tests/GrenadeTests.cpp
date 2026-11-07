#include "../src/combat/GrenadeProjectile.hpp"
#include "../src/combat/GrenadeThrow.hpp"
#include "../src/combat/GrenadeLaunch.hpp"
#include <cassert>
#include "../src/combat/WeaponSlots.hpp"
#include "../src/combat/SmokeVisibility.hpp"
#include "../src/presentation/WeaponAnimation.hpp"

int main() {
    GrenadeVisual cloud={{0,0,0},WeaponSlots::Smoke,300,.99f,2,true};
    assert(std::abs(SmokeVisibility::Length(cloud,{-500,0,150},{500,0,150})-600)<.001f);
    assert(std::abs(SmokeVisibility::Length(cloud,{0,0,150},{100,0,150})-100)<.001f);
    assert(SmokeVisibility::Length(cloud,{-500,300,150},{500,300,150})==0);
    assert(SmokeVisibility::Length(cloud,{-500,0,600},{500,0,600})==0);
    assert(!SmokeVisibility::Visible({cloud},{0,0,150},{100,0,150}));
    assert(SmokeVisibility::Visible({cloud},{0,0,150},{20,0,150}));
    assert(!SmokeVisibility::Visible({cloud},{500,0,150},{-500,0,150},true));
    assert(!SmokeVisibility::Visible({cloud},{-500,0,150},{500,0,150},true));
    float once=SmokeVisibility::Transmission({cloud},{0,0,150},{30,0,150});
    assert(SmokeVisibility::Transmission({cloud,cloud},{0,0,150},{30,0,150})<once);
    cloud.blockAI=false;assert(SmokeVisibility::Visible({cloud},{0,0,150},{500,0,150},true));
    assert(!SmokeVisibility::Visible({cloud},{0,0,150},{500,0,150}));
    cloud.opacity=0;assert(SmokeVisibility::Visible({cloud},{0,0,150},{500,0,150}));

    GrenadeProjectile level;assert(GrenadeLaunch::Prepare(level,{0,0,96},{1,0,0},{}));
    // Stock CS level release: 600 CS units/s at ten degrees, plus a 16-unit forward offset, scaled 1.5.
    assert(std::abs(level.velocity[0]-886.32697f)<.01f && std::abs(level.velocity[2]-156.28336f)<.01f);
    assert(std::abs(level.position[0]-23.63539f)<.01f && std::abs(level.position[2]-100.16756f)<.01f);
    auto original=level;bool landed=false;
    for(int step=1;step<120;++step) {
        level.Advance(step/120.,{},[](float,float){return 0.f;});
        if(level.bounced){landed=true;break;}
    }
    assert(landed && level.position[0]>750 && level.position[0]<780);
    // Upward aim reaches the stock speed cap; sideways and jumping momentum add without normalization loss.
    GrenadeProjectile high;assert(GrenadeLaunch::Prepare(high,{0,0,96},{.70710678f,0,.70710678f},{100,20,30}));
    assert(std::abs(high.velocity[0]-100-723.13606f)<.02f && std::abs(high.velocity[2]-30-861.79999f)<.02f && high.velocity[1]==20);
    assert(GrenadeLaunch::Prepare(high,{0,0,96},{0,1,0},{}));assert(std::abs(high.velocity[1]-original.velocity[0])<.01f);
    // Downward view uses CS's other pitch branch and reduces launch speed instead of keeping a fixed arc.
    assert(GrenadeLaunch::Prepare(high,{0,0,96},{.70710678f,0,-.70710678f},{}));
    assert(std::abs(high.velocity[0]-344.71999f)<.02f && std::abs(high.velocity[2]+289.25442f)<.02f);
    assert(!GrenadeLaunch::Prepare(high,{0,0,96},{0,0,0},{}));
    // The forward hand offset cannot spawn through a close wall or a cliff face.
    Bounds3 releaseWall={{12,-50,0},{14,50,200}};
    GrenadeLaunch::ClipRelease(original,{0,0,96},{releaseWall},[](float,float){return 0.f;});
    assert(original.position[0]<8);
    assert(GrenadeLaunch::Prepare(high,{0,0,96},{1,0,0},{}));
    GrenadeLaunch::ClipRelease(high,{0,0,96},{},[](float x,float){return x>=12 ? 200.f : 0.f;});
    assert(high.position[0]<12);
    GrenadeThrow action;using E=GrenadeThrow::Event;
    assert(action.Step(0,true,true,1,.25f,.8f)==E::Pull);
    assert(action.Step(10,false,true,1,.25f,.8f)==E::None); // A held pin never starts a fuse.
    assert(action.Step(10.1,false,false,1,.25f,.8f)==E::Throw);
    assert(action.Step(10.2,false,false,1,.25f,.8f)==E::None);
    assert(action.Step(10.36,false,false,1,.25f,.8f)==E::Launch);
    assert(action.Step(10.4,false,false,1,.25f,.8f)==E::None);
    assert(action.Step(11,false,false,1,.25f,.8f)==E::Finished);
    assert(action.Step(12,true,false,1,.25f,.8f)==E::Pull);action.Reset();assert(!action.Active());
    // An immediate throw owns the recovery pose and cannot emit a second launch on a held button.
    action.StartRecovery(20);
    assert(action.Step(20.8,true,true,0,0,.9f)==E::None);
    assert(action.Step(20.91,false,true,0,0,.9f)==E::Finished);
    assert(WeaponAnimation::HoldLastFrame("pinpull") && WeaponAnimation::HoldLastFrame("throw2"));
    // A held pin must never sample the two offscreen frames or fall back to idle.
    assert(WeaponAnimation::HeldActionFrame("pinpull",12)==12);
    assert(WeaponAnimation::HeldActionFrame("pinpull",24)==22);
    assert(WeaponAnimation::HeldActionFrame("throw2",18)==18);
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
    // A satchel stops at first ground contact. A wall removes normal velocity without suspending it.
    GrenadeProjectile satchel;satchel.settleOnGround=true;satchel.position={0,0,96};satchel.velocity={400,0,30};
    satchel.Advance(.5,{box},ground);
    assert(!satchel.settled && !satchel.bounced && satchel.position[0]<96 && satchel.velocity[0]==0 && satchel.velocity[2]<0);
    satchel.Advance(1,{box},ground);
    assert(satchel.settled && !satchel.bounced && satchel.position[2]==4 && satchel.velocity==GrenadePoint{});
    saved=satchel.position;satchel.Advance(20,{box},ground);assert(saved==satchel.position);
    GrenadeProjectile falling;falling.settleOnGround=true;falling.position={101,0,240};
    falling.Advance(1,{box},ground);
    assert(falling.settled && !falling.bounced && falling.position[2]>203.9f && falling.position[2]<204.2f);
    assert(GrenadeProjectile::Falloff(0,350)==1 && GrenadeProjectile::Falloff(400,350)==0);
    GrenadePoint flash={100,0,0},eye={0,0,0};
    assert(GrenadeProjectile::Exposure(flash,eye,{1,0,0},1000)>GrenadeProjectile::Exposure(flash,eye,{-1,0,0},1000)*4);
}
