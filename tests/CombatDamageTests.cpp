#include "../src/combat/CombatDamage.hpp"
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
    // Changing the percentage must affect every allied damage source without changing enemy finishing/damage.
    Require(CombatDamage::Amount(36,true,false,420,25)==9,"quarter-damage allied AK");
    Require(CombatDamage::Amount(240,true,false,500,25)==60,"quarter-damage heavy melee");
    Require(CombatDamage::Amount(2500,true,false,5000,25)==625,"quarter-damage allied C4");
    Require(CombatDamage::Amount(115,true,true,420,0)==0,"zero friendly AWP even with a finishing flag");
    Require(CombatDamage::Amount(2500,true,false,5000,0)==0,"C4 friendly damage can be disabled");
    Require(CombatDamage::Amount(36,true,false,420,100)==36,"full allied damage");
    Require(CombatDamage::Amount(36,true,false,420,-10)==0,"negative percentages cannot heal allies");
    Require(CombatDamage::Amount(36,true,false,420,200)==36,"allied percentage clamps at full damage");
    Require(CombatDamage::Amount(2500,false,false,5000,0)==2500,"enemy C4 independent of allied percent");
    Require(CombatDamage::Amount(115,false,true,5000,0)>5000,"enemy AWP finishing independent of allied percent");
    std::puts("Combat damage invariants passed: friendly coefficient, C4 and AWP finishing");
}
