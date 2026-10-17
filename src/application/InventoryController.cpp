#include "InventoryController.hpp"
#include <algorithm>

void InventoryController::ClearInput() {
    // F8, view changes and map exit may close the panel while LMB still owns capture outside the window.
    bool pointer=controls_.Open() || controls_.RoutingOpen();controls_.Reset(controls_.Numbers());
    if(pointer && GetCapture())ReleaseCapture();
}
void InventoryController::ApplySettings(const GameplaySettings& settings) {
    // Only startup/F8/map reset restore the default. Closing I, changing view or losing focus preserves Tab.
    ClearInput();defaultNumbers_=settings.inventoryControls && settings.inventoryNumbersDefault;
    controls_.Reset(defaultNumbers_);
}
const char* InventoryController::Update(wc3::Handle actor,const GameplaySettings& settings,bool blocked,float yaw,bool self,
    const Aim& pick,const std::function<void()>& prepare) {
    actions_.Update();
    if(blocked || !settings.inventoryControls) {
        ClearInput();if(!settings.inventoryControls)controls_.Reset(false);actions_.Cancel();return nullptr;
    }
    // Map triggers/consumption may replace an item while the mouse is held or a target is being chosen.
    if((controls_.PressedSlot()>=0 && wc3::UnitItemInSlot(actor,controls_.PressedSlot())!=controls_.PressedItem()) ||
        (controls_.TargetSlot()>=0 && wc3::UnitItemInSlot(actor,controls_.TargetSlot())!=controls_.TargetItem()))controls_.Cancel();
    const char* notice=nullptr;InventoryControls::Event e;
    auto use=[&](int slot,wc3::Handle item,const SpellAim* aim) {
        prepare();auto result=actions_.Use(actor,slot,item,aim,e.self || self);
        if(result==NativeItemActions::Result::TargetRequired) {controls_.Target(slot,item);notice="ITEM: SELECT A TARGET | RMB: CANCEL";}
        else if(result==NativeItemActions::Result::Issued) {controls_.Cancel();notice="ITEM ORDER ISSUED";}
        else if(result==NativeItemActions::Result::Requested) {controls_.Cancel();notice="ITEM TARGET SENT";}
        else notice="ITEM UNAVAILABLE: CHECK TARGET, COOLDOWN OR PASSIVE";
    };
    while(controls_.Next(e)) {
        using Kind=InventoryControls::Kind;
        auto layout=InventoryHudLayout::Calculate(e.width,e.height,settings.spellIconSizePercent,settings.showSpellIcons,true);
        int slot=layout.PointerSlot(e.x,e.y);
        if(e.kind==Kind::Numbers) {controls_.ToggleNumbers();wc3::Log("Inventory numbers enabled=%d",controls_.Numbers());}
        else if(e.kind==Kind::Panel) {controls_.TogglePanel();if(controls_.Open())prepare();wc3::Log("Inventory mouse open=%d",controls_.Open());}
        else if(e.kind==Kind::Close)controls_.Close();
        else if(e.kind==Kind::Cancel)controls_.Cancel();
        else if(e.kind==Kind::Use) {
            if(e.slot>=0 && e.slot<6) {auto aim=pick(-1,-1,0,0);use(e.slot,wc3::UnitItemInSlot(actor,e.slot),&aim);}
        } else if(controls_.Open()) {
            int threshold=std::max(4,GetSystemMetrics(SM_CXDRAG));
            if(e.kind==Kind::Down) {
                if(controls_.TargetSlot()>=0 && slot==-1) {
                    auto aim=pick(e.x,e.y,e.width,e.height);use(controls_.TargetSlot(),controls_.TargetItem(),&aim);
                } else controls_.Down(slot,slot>=0 ? wc3::UnitItemInSlot(actor,slot) : 0,e.x,e.y);
            } else if(e.kind==Kind::Move)controls_.Motion(e.x,e.y,threshold);
            else if(e.kind==Kind::Up && controls_.PressedSlot()>=0) {
                auto action=controls_.Up(slot,e.x,e.y,threshold);
                if(action.kind==InventoryControls::Command::Use)use(action.slot,action.item,nullptr);
                else if(action.kind!=InventoryControls::Command::None) {
                    prepare();bool accepted=action.kind==InventoryControls::Command::Drop ?
                        actions_.Drop(actor,action.slot,action.item,yaw,settings.inventoryDropDistance) : actions_.Move(actor,action.slot,action.item,action.destination);
                    notice=accepted ? "ITEM ORDER ISSUED" : "CANNOT MOVE OR DROP THIS ITEM";
                }
            }
        }
    }
    return notice;
}
