#include "NativeItemActions.hpp"
#include "NativeItemCommands.hpp"
#include <cmath>

bool NativeItemActions::Valid(wc3::Handle unit,int slot,wc3::Handle expected) const {
    return unit && wc3::GetUnitTypeId(unit) && slot>=0 && slot<6 && slot<wc3::UnitInventorySize(unit) &&
        expected && wc3::UnitItemInSlot(unit,slot)==expected;
}
void NativeItemActions::Cancel() {
    if(order_ && unit_ && wc3::GetUnitTypeId(unit_))wc3::IssueImmediateOrderById(unit_,851972);
    Reset();
}
void NativeItemActions::Update() {
    if(order_ && (!wc3::GetUnitTypeId(unit_) || (GetTickCount()-requested_>=100 && wc3::GetUnitCurrentOrder(unit_)!=order_))) {
        wc3::Log("Inventory order finished order=%d elapsedMs=%lu",order_,GetTickCount()-requested_);Reset();
    }
}
NativeItemActions::Result NativeItemActions::Use(wc3::Handle unit,int slot,wc3::Handle expected,const SpellAim* aim,bool self) {
    if(!Valid(unit,slot,expected))return Result::Rejected;
    unsigned forms=NativeItemCommands::Forms(base_,unit,slot,expected);
    wc3::Log("Inventory use slot=%d item=%08X forms=%u target=%08X point=%d self=%d",slot+1,expected,forms,self ? unit : aim ? aim->target : 0,aim && aim->point,self);
    if(!forms)return Result::Rejected;
    if(!(forms&1) && !aim)return Result::TargetRequired;
    Cancel();Begin(unit,852008+slot);BOOL accepted=FALSE;bool targeted=false;
    auto dispatched=[&](BOOL reported) {
        // 1.26a's item point/widget wrappers can enqueue a valid order and still return false when the card has no entry.
        int current=wc3::GetUnitCurrentOrder(unit);
        wc3::Log("Inventory native dispatch reported=%d current=%d wanted=%d",reported,current,order_);
        return reported || current==order_;
    };
    if(forms&1)accepted=dispatched(wc3::UnitUseItem(unit,expected));
    else if(aim) {
        if((forms&4) && (self || aim->target)) {targeted=true;accepted=dispatched(wc3::UnitUseItemTarget(unit,expected,self ? unit : aim->target));}
        else if((forms&2) && (aim->point || self)) {
            float x=self ? wc3::Real(wc3::GetUnitX(unit)) : aim->x,y=self ? wc3::Real(wc3::GetUnitY(unit)) : aim->y;
            targeted=true;accepted=dispatched(wc3::UnitUseItemPoint(unit,expected,&x,&y));
        }
    }
    wc3::Log("Inventory use result slot=%d item=%08X accepted=%d order=%d",slot+1,expected,accepted,order_);
    // Missing command-card entries make the targeted wrappers return false even after queueing.
    // Preserve one native dispatch window; Update observes the real order, never inventing item effects/cooldowns.
    if(targeted && !accepted)return Result::Requested;
    if(!accepted)Reset();return accepted ? Result::Issued : Result::Rejected;
}
bool NativeItemActions::Drop(wc3::Handle unit,int slot,wc3::Handle expected,float yaw,float distance) {
    if(!Valid(unit,slot,expected) || !std::isfinite(yaw) || !std::isfinite(distance))return false;
    // Releasing outside the inventory drops beside the actor, without a distant walk-to order.
    float x=wc3::Real(wc3::GetUnitX(unit))+std::cos(yaw*.01745329252f)*distance;
    float y=wc3::Real(wc3::GetUnitY(unit))+std::sin(yaw*.01745329252f)*distance;
    Cancel();Begin(unit,0xD0021);BOOL accepted=wc3::UnitDropItemPoint(unit,expected,&x,&y);
    wc3::Log("Inventory drop slot=%d item=%08X accepted=%d x=%.1f y=%.1f",slot+1,expected,accepted,x,y);
    if(!accepted)Reset();return accepted!=FALSE;
}
bool NativeItemActions::Move(wc3::Handle unit,int slot,wc3::Handle expected,int destination) {
    if(!Valid(unit,slot,expected) || destination<0 || destination>=wc3::UnitInventorySize(unit) || destination>=6)return false;
    Cancel();Begin(unit,0xD0022+destination);BOOL accepted=wc3::UnitDropItemSlot(unit,expected,destination);
    wc3::Log("Inventory move slot=%d destination=%d item=%08X accepted=%d",slot+1,destination+1,expected,accepted);
    if(!accepted)Reset();return accepted!=FALSE;
}
