#include "../src/presentation/CombatVisuals.hpp"
#include <cassert>
#include <cstdio>
#include <limits>
#include <cstdlib>
int main(){
    _set_error_mode(_OUT_TO_STDERR);
    CombatVisuals effects;CombatVisualSettings s;CombatVisuals::Point eye={0,0,96},ray={1,0,0},velocity={0,0,0},contact={160,0,96},above={160,0,120};
    for(int weapon=0;weapon<WeaponSlots::Count;++weapon)effects.Eject(weapon,eye,ray,velocity,0,s);
    assert(effects.Shells().size()==11); // Conventional firearms only, one shell per round, no energy/explosive/melee cases.
    int red=0;for(const auto& shell:effects.Shells()){red+=shell.shotgun;assert(shell.position[0]==20 && shell.position[1]==-10 && shell.position[2]==84);}
    assert(red==2);
    // Bonus/text opt-out still emits blood at the accepted head contact, never at the feet or caption anchor.
    effects.Reset();effects.Headshot(9,contact,above,ray,s,false);
    assert(effects.Criticals().empty() && effects.BloodDrops().size()==s.bloodCount);
    for(const auto& drop:effects.BloodDrops())assert(drop.position==contact);
    effects.Reset();for(int i=0;i<3;++i)effects.Eject(WeaponSlots::Glock,eye,ray,velocity,0,s);assert(effects.Shells().size()==3);
    for(int i=0;i<200;++i)effects.Eject(0,eye,ray,velocity,0,s);assert(effects.Shells().size()==64);
    effects.Headshot(5,contact,above,ray,s);assert(effects.BloodDrops().size()==16 && effects.Criticals().size()==1);
    effects.Headshot(5,contact,above,ray,s);assert(effects.Criticals().size()==1); // Pellet spam is coalesced, not damage.
    effects.Step(.1f);effects.Headshot(5,contact,above,ray,s);assert(effects.Criticals().size()==2);
    float age=effects.Shells()[0].age;effects.Step(0);effects.Step(std::numeric_limits<float>::quiet_NaN());assert(effects.Shells()[0].age==age);
    effects.Step(1.1f);assert(effects.BloodDrops().empty() && effects.Criticals().empty());
    for(const auto& shell:effects.Shells())assert(shell.position[2]>=shell.floor+shell.size);
    effects.Step(3);assert(effects.Shells().empty());
    s.headshotBlood=s.criticalText=s.shellCasings=false;effects.Headshot(6,contact,above,ray,s);effects.Eject(0,eye,ray,velocity,0,s);
    assert(effects.BloodDrops().empty() && effects.Criticals().empty() && effects.Shells().empty());
    s=CombatVisualSettings{};CombatVisuals slow,fast;slow.Eject(0,eye,ray,velocity,0,s);fast.Eject(0,eye,ray,velocity,0,s);
    for(int i=0;i<30;++i)slow.Step(1.f/60);for(int i=0;i<72;++i)fast.Step(1.f/144);
    assert(std::abs(slow.Shells()[0].position[2]-fast.Shells()[0].position[2])<2);
    slow.Reset();assert(slow.Shells().empty() && slow.Criticals().empty());
    std::puts("Blood/critical confirmation pools, pellet throttling, casing families, burst counts, bounds, arcs/bounces, pause, expiry and opt-outs passed.");
}
