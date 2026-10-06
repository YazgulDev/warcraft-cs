#include "../src/WeaponRecoil.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static void Require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
int main() {
    // Numerical oracles use classic CS coefficients, not approximate stance comparisons alone.
    WeaponRecoil ak, m4, crouch, moving, air;
    ak.Shot(0,0,true,false); m4.Shot(1,0,true,false);
    Require(std::abs(ak.Pitch()-1)<.001f && std::abs(m4.Pitch()-.65f)<.001f,"AK and M4 first-shot kick must differ");
    Require(ak.Yaw()!=0,"rifle recoil must affect lateral bullet aim");
    Require(std::abs(ak.Yaw()+.375f)<.001f,"a fresh CS weapon initially kicks left");
    crouch.Shot(0,0,true,true); moving.Shot(0,375,true,false); air.Shot(0,0,false,false);
    Require(crouch.Pitch()<ak.Pitch() && moving.Pitch()>ak.Pitch() && air.Pitch()>moving.Pitch(),"stance must change kick");
    WeaponRecoil crouchWalking, movingAir;
    crouchWalking.Shot(0,1,true,true);movingAir.Shot(0,1,false,false);
    Require(crouchWalking.Pitch()==1.5f && movingAir.Pitch()==1.5f,"CS moving branch precedes duck and airborne branches");
    ak.Shot(0,0,true,false);
    Require(std::abs(ak.Pitch()-2.35f)<.001f,"second AK shot uses 1 + 2*0.175 growth");
    ak.Reset();ak.Shot(0,0,true,false);
    float initial=ak.Pitch();
    for(int i=0;i<15;++i) {ak.Step(.1f,true,0);ak.Shot(0,0,true,false);}
    Require(ak.Pitch()>initial && ak.Pitch()<=5.751f,"sustained spray must accumulate and cap");
    for(int i=0;i<120;++i)ak.Step(1.0f/60,false,0);
    Require(ak.Magnitude()==0 && ak.Shots()==0,"release must return to mouse aim and reset the burst");
    WeaponRecoil awp, usp;
    awp.Shot(3,0,true,true);usp.Shot(2,400,false,false);
    Require(awp.Pitch()==2 && usp.Pitch()==2 && awp.Shots()==0,"AWP/USP original two-degree kick does not use rifle stances");
    awp.Step(.1f,false,3);
    Require(std::abs(awp.Pitch()-.9f)<.00001f,"PM_DropPunchAngle subtracts (10 + length*0.5)*dt exactly");
    usp.Step(.01f,false,2);Require(usp.Shots()==0,"USP release clears its shot count immediately");
    WeaponRecoil burst;
    for(int i=0;i<30;++i)burst.Shot(0,0,true,false);
    burst.Step(.01f,false,0);Require(burst.Shots()==15,"first release caps rifle penalty at fifteen shots");
    burst.Step(.39f,false,0);Require(burst.Shots()==15,"release delay retains the shot count for 400ms");
    burst.Step(.5f,false,0);Require(burst.Shots()==14,"classic recovery decreases at most one shot per command");
    burst.Step(.02f,false,0);Require(burst.Shots()==14,"next decrement waits 22.5ms");
    burst.Step(.003f,false,0);Require(burst.Shots()==13,"shot penalty decays after 22.5ms");
    burst.Shot(0,0,true,false);float punch=burst.Pitch();burst.ResetBurst();
    Require(burst.Shots()==0 && burst.Pitch()==punch,"reload/deploy resets burst while retaining player punch");
    // Published ran1 seed-one sequence verifies the shuffled generator independently of rifle coefficients.
    GoldSrcRandom random;random.Reset(1);
    const unsigned expected[]={893351816,197493099,1624379149,1137522503,1998097157,823564440};
    for(unsigned value:expected)Require(random.Uniform(2147483646u)==value,"GoldSrc shuffled RNG sequence");
    burst.Reset();burst.Shot(4,0,true,false);burst.Shot(5,0,true,false);burst.Shot(6,0,true,false);
    Require(burst.Magnitude()==0,"knife, C4 and sword must never kick the gun camera");
    std::puts("Classic CS recoil oracles passed: stance coefficients/precedence, growth/caps, discrete punch recovery, burst timers, reload, GoldSrc RNG, melee");
}
