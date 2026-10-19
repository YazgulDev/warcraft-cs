#include "../src/combat/RocketProjectile.hpp"
#include <cassert>
#include <cstdio>

int main() {
    auto ground=[](float,float){return 0.f;};RocketProjectile rocket;
    assert(rocket.Launch({0,0,96},{1,0,0},{100,20,30},{},ground));
    assert(std::abs(rocket.velocity[0]-424.7595f)<.01f && std::abs(rocket.velocity[2]-187.5f)<.01f && rocket.velocity[1]==0);
    assert(std::abs(rocket.position[0]-24)<.1f && std::abs(rocket.position[1]+12)<.1f && std::abs(rocket.position[2]-84)<.1f);
    rocket.Advance(.3,{},ground);assert(!rocket.done && rocket.age<rocket.settings.ignition);
    rocket.Advance(1.2,{},ground);assert(!rocket.done && RocketProjectile::Length(rocket.velocity)>2000 && RocketProjectile::Length(rocket.velocity)<=3000.1f);
    // Motor pulses and swept collision must behave consistently at different presentation rates.
    RocketProjectile a,b;assert(a.Launch({0,0,96},{1,0,0},{},{},ground));b=a;
    for(int i=1;i<=60;++i)a.Advance(i/30.,{},ground);
    for(int i=1;i<=288;++i)b.Advance(i/144.,{},ground);
    for(int i=0;i<3;++i)assert(std::abs(a.position[i]-b.position[i])<2);
    RocketProjectile guided;assert(guided.Launch({0,0,96},{1,0,0},{},{},ground));GrenadePoint spot={2500,1000,200};
    guided.Advance(1.2,{},ground,&spot);assert(guided.velocity[1]>300 && guided.position[1]>100);
    Bounds3 wall={{800,-100,0},{802,100,300}};RocketProjectile blocked;assert(blocked.Launch({0,0,96},{1,0,0},{},{},ground));
    blocked.Advance(2,{wall},ground);assert(blocked.done && blocked.position[0]<800 && blocked.position[0]>795);
    auto saved=blocked.position;blocked.Advance(8,{},ground);assert(saved==blocked.position);
    // The launcher cannot place a fast projectile through a close wall or fire an invalid direction.
    Bounds3 close={{10,-100,0},{12,100,300}};assert(blocked.Launch({0,0,96},{1,0,0},{},{close},ground));assert(blocked.done && blocked.position[0]<10);
    assert(!blocked.Launch({0,0,96},{},{},{},ground));
    RocketProjectile limited;limited.range=200;assert(limited.Launch({0,0,96},{1,0,0},{},{},ground));limited.Advance(5,{},ground);assert(limited.done && limited.travelled<=200.1f);
    RocketProjectile fast;fast.settings.launchSpeed=6000;fast.settings.maximumSpeed=100;
    assert(fast.Launch({0,0,96},{1,0,0},{},{},ground));assert(fast.Reach(1)>6000);
    std::puts("PASS RPG launch/boost/coast, movement, guidance, frame-rate tolerance, swept walls and lifetime/range ownership.");
}
