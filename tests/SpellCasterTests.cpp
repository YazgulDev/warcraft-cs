#include "../src/platform/SpellCaster.hpp"
#include "../src/platform/MapSpellCharges.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>

namespace {int level=1,current=0,stops=0;bool unitAccepted=false,pointAccepted=false,immediateAccepted=false;
int calls=0,unitCalls=0,pointCalls=0,unitType=1,chargeMarker=-1;bool pointOnly=false;wc3::Handle lastTarget=0;float lastX=0;SpellCaster* activeCaster=nullptr;SpellHudSlot state;}
namespace wc3 {
void Log(const char*,...) {}
float Real(Bits) {return 100;}
int (__cdecl* GetUnitAbilityLevel)(Handle,int)=[](Handle,int ability) {
    return ability>=0x41325932 && ability<=0x41325938 ? int(ability==0x41325932+chargeMarker) : level;};
int (__cdecl* GetUnitTypeId)(Handle)=[](Handle) {return unitType;};
int (__cdecl* GetUnitCurrentOrder)(Handle)=[](Handle) {return current;};
Bits (__cdecl* GetUnitState)(Handle,int)=[](Handle,int)->Bits {return 0;};
Bits (__cdecl* GetUnitX)(Handle)=[](Handle)->Bits {return 0;};
Bits (__cdecl* GetUnitY)(Handle)=[](Handle)->Bits {return 0;};
BOOL (__cdecl* IssueTargetOrderById)(Handle,int,Handle)=[](Handle,int order,Handle target)->BOOL {
    // The native motor can consult ownership synchronously inside this dispatch.
    assert(activeCaster && activeCaster->Active());++calls;++unitCalls;lastTarget=target;if(unitAccepted)current=order;return unitAccepted;};
BOOL (__cdecl* IssuePointOrderById)(Handle,int,float*,float*)=[](Handle,int order,float* x,float*)->BOOL {++calls;++pointCalls;lastX=*x;if(pointAccepted)current=order;return pointAccepted;};
BOOL (__cdecl* IssueImmediateOrderById)(Handle,int)=[](Handle,int order)->BOOL {
    if(order==851972) {++stops;current=0;return TRUE;}++calls;if(immediateAccepted)current=order;return immediateAccepted;};
}
int NativeSpellSlots::Ability(wc3::Handle,int slot,const SpellSettings&,bool) {return slot==3 ? 0 : slot+1;}
int NativeSpellSlots::Order(wc3::Handle,int ability) {return ability==3 ? 0 : 852095;}
bool NativeSpellSlots::PointOnly(wc3::Handle,int,int) {return pointOnly;}
SpellHudSlots NativeSpellSlots::Snapshot(wc3::Handle,const SpellSettings&) {return {state,state,state,state};}
int main() {
    SpellCaster caster;SpellSettings settings;SpellAim aim;aim.target=7;aim.point=true;aim.x=120;
    activeCaster=&caster;
    unitAccepted=true;caster.Cast(1,0,settings,aim,false);assert(caster.Active() && lastTarget==7 && calls==1);
    // An active native order survives updates; only a deliberate cancel sends stop.
    caster.Update();assert(caster.Active() && stops==0);caster.Cancel();assert(!caster.Active() && stops==1);
    unitAccepted=false;pointAccepted=true;calls=0;caster.Cast(1,0,settings,aim,false);assert(caster.Active() && calls==2 && lastX==120);
    caster.Reset();assert(!caster.Active() && stops==1); // Map reset never accesses unloaded actor handles.
    pointAccepted=false;immediateAccepted=true;calls=0;caster.Cast(1,0,settings,{},false);assert(calls==1 && caster.Active());caster.Cancel();
    unitAccepted=true;caster.Cast(1,0,settings,aim,true);assert(lastTarget==1);caster.Cancel();
    unitAccepted=pointAccepted=immediateAccepted=false;calls=0;caster.Cast(1,0,settings,aim,false);assert(!caster.Active() && calls==3);
    // Blink/Flame Strike must ignore a nearby widget even when Warcraft would accept its position as a target.
    pointOnly=true;unitAccepted=pointAccepted=true;unitCalls=pointCalls=calls=0;
    caster.Cast(1,0,settings,aim,false);assert(caster.Active() && calls==1 && pointCalls==1 && !unitCalls && lastX==120);caster.Cancel();
    pointAccepted=false;calls=unitCalls=pointCalls=0;
    caster.Cast(1,0,settings,aim,false);assert(!caster.Active() && calls==1 && !unitCalls);
    calls=0;auto sky=aim;sky.point=false;caster.Cast(1,0,settings,sky,false);assert(!calls && !caster.Active());
    pointAccepted=true;caster.Cast(1,0,settings,sky,true);assert(caster.Active() && !unitCalls && lastX==100);caster.Cancel();
    // Explicit per-slot target overrides keep their established meaning.
    settings.target[0]="unit";caster.Cast(1,0,settings,aim,false);assert(unitCalls==1 && lastTarget==7);caster.Cancel();
    settings.target[0]="auto";pointOnly=false;unitAccepted=pointAccepted=immediateAccepted=false;calls=0;
    level=0;calls=0;caster.Cast(1,0,settings,aim,false);assert(calls==0 && !caster.Active());level=1;
    caster.Cast(1,2,settings,aim,false);caster.Cast(1,3,settings,aim,false);caster.Cast(1,-1,settings,aim,false);assert(calls==0);
    settings.enabled=false;caster.Cast(1,0,settings,aim,false);assert(calls==0);
    settings.enabled=true;settings.target[0]="unit";caster.Cast(1,0,settings,{},false);assert(calls==0);
    settings.target[0]="immediate";immediateAccepted=true;caster.Cast(1,0,settings,aim,false);assert(calls==1 && caster.Active());
    Sleep(110);current=0;caster.Update();assert(!caster.Active());
    // Unavailable native casts cannot dispatch or interrupt an already active channel.
    state.cooldownKnown=true;state.cooldown=12;calls=0;
    assert(!strcmp(caster.Cast(1,0,settings,aim,false),"SPELL ON COOLDOWN") && calls==0);
    // A transformed ability can have an unsupported order getter while its original timer still runs.
    assert(!strcmp(caster.Cast(1,2,settings,aim,false),"SPELL ON COOLDOWN") && calls==0);
    state.cooldown=0;state.charges=0;
    assert(!strcmp(caster.Cast(1,0,settings,aim,false),"NO SPELL CHARGES") && calls==0);
    state.charges=1;caster.Cast(1,0,settings,aim,false);assert(calls==1 && caster.Active());
    state.noMana=true;int beforeStops=stops;calls=0;
    caster.Cast(1,0,settings,aim,false);assert(calls==0 && stops==beforeStops && caster.Active());
    caster.Cancel();
    // Map charge markers, including empty, are read without changing abilities or starting local timers.
    unitType=0x5573796C;
    for(int charge=0;charge<=3;++charge) {
        SpellHudSlot marker;marker.ability=0x41303634;marker.level=1;chargeMarker=charge;
        MapSpellCharges::Read(1,marker);assert(marker.charges==charge);
    }
    SpellHudSlot unknown;unknown.ability=0x41303634;unknown.level=1;chargeMarker=-1;
    MapSpellCharges::Read(1,unknown);assert(unknown.charges==-1);
    chargeMarker=3;unitType=1;MapSpellCharges::Read(1,unknown);assert(unknown.charges==-1);
    std::puts("PASS native spell target forms, self cast, passive/unlearned rejection, order ownership and reset.");
}
