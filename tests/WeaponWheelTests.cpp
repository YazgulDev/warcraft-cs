#include "../src/input/WeaponWheel.hpp"
#include <cstdio>
#include <cstdlib>
void Require(bool condition,const char* message) {
    if (!condition) { std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1); }
}
int main() {
    WeaponWheel wheel;
    // Keep the optional legacy slot wrapper while the selector consumes exact signed detents.
    for (int slot=0;slot<WeaponSlots::Count;++slot) {
        wheel.Add(120);Require(wheel.Take(slot)==(slot+WeaponSlots::Count-1)%WeaponSlots::Count,"Wheel up / first-slot wrap failed");
        wheel.Add(-120);Require(wheel.Take(slot)==(slot+1)%WeaponSlots::Count,"Wheel down / last-slot wrap failed");
    }
    wheel.Add(-360);Require(wheel.Take(0)==3,"Three fast notches must select AWP");
    Require(wheel.Take(3)==3,"Wheel input must not replay on later frames");
    wheel.Add(45);Require(wheel.Take(2)==2,"Partial notch must not switch early");
    wheel.Add(45);Require(wheel.Take(2)==2,"High-resolution fractions must accumulate");
    wheel.Add(30);Require(wheel.Take(2)==1,"One accumulated notch must switch once");
    wheel.Add(-60);wheel.Reset();wheel.Add(-60);
    Require(wheel.Take(1)==1,"Focus/status/map reset must discard old wheel fractions");
    wheel.Reset();wheel.Add(120);wheel.Add(-120);Require(wheel.Take(4)==4,"Opposite queued notches must cancel");
    wheel.Add(-120*WeaponSlots::Count);Require(wheel.Take(4)==4,"Complete legacy slot cycle must wrap");
    wheel.Add(-1080);Require(wheel.TakeSteps()==9,"Nine forward detents must not become eight backward detents");
    wheel.Add(1080);Require(wheel.TakeSteps()==-9,"Nine backward detents must preserve direction");
    wheel.Add(-120*WeaponSlots::Count);Require(wheel.TakeSteps()==WeaponSlots::Count,"A full slot lap must remain available for sparse inventories");
    wheel.Add(45);Require(wheel.TakeSteps()==0,"Raw detents retain fractions");
    wheel.Add(75);Require(wheel.TakeSteps()==-1 && wheel.TakeSteps()==0,"Fractions complete one detent and never replay");
    std::puts("PASS signed detents, legacy slot wrap, fast batches, fractional packets, one-shot consumption and reset");
}
