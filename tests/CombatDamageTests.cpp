#include "../src/CombatDamage.hpp"
#include <cstdio>
#include <cstdlib>

static void Require(bool condition,const char* message) {
    if (!condition) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
int main() {
    // Friendly fire must halve base damage without imposing a fixed cap or applying AWP's finishing fallback.
    Require(CombatDamage::Amount(36,true,false,420)==18,"friendly AK is half base damage");
    Require(CombatDamage::Amount(240,true,false,500)==120,"friendly heavy sword exceeds 50 but remains half");
    Require(CombatDamage::Amount(2500,true,false,5000)==1250,"friendly C4 halves fixed damage");
    Require(CombatDamage::Amount(2500,false,false,5000)==2500,"enemy C4 keeps fixed damage");
    Require(!CombatDamage::FinishesTarget(true,true),"friendly AWP never invokes finishing");
    Require(CombatDamage::Amount(115,true,false,420)==57.5f,"friendly AWP uses ordinary bullet damage");
    Require(CombatDamage::FinishesTarget(true,false),"enemy AWP retains its one-hit rule");
    Require(CombatDamage::Amount(115,false,true,5000)>5000,"enemy AWP scales with target health");
    std::puts("Combat damage invariants passed: friendly coefficient, C4 and AWP finishing");
}
