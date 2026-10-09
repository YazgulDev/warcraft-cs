#include "../src/input/WeaponWheel.hpp"
#include <cstdio>
#include <cstdlib>
void Require(bool condition,const char* message) {
    if (!condition) { std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1); }
}
int main() {
    WeaponWheel wheel;
    // Windows' signed deltas use the same seven slots as the direct-selection keys.
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
    wheel.Add(-120*WeaponSlots::Count);Require(wheel.Take(4)==4,"Complete seven-slot cycle must wrap");
    std::puts("PASS wheel direction, all seven slots, wraparound, fast scroll, fractional packets, one-shot consumption and reset");
}
