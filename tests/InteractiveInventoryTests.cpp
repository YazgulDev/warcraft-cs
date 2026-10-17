#include "../src/application/InventoryController.hpp"
#include "../src/platform/NativeItemCommands.hpp"
#include "../src/presentation/CursorAimRay.hpp"
#include "../src/presentation/HealthBarView.hpp"
#include <cassert>
#include <cmath>

namespace {
wc3::Handle items[]={11,22,33,44,55,66};unsigned forms=1;int uses=0,points=0,targets=0,drops=0,moves=0,stops=0,charges=3,current=0;
bool allowed=true;float px=0,py=0;wc3::Handle target=0;NativeItemActions* observing=nullptr;
int __cdecl Type(wc3::Handle unit) {return unit ? 123 : 0;}
int __cdecl Size(wc3::Handle) {return 6;}
wc3::Handle __cdecl Slot(wc3::Handle,int slot) {return slot>=0 && slot<6 ? items[slot] : 0;}
int __cdecl Order(wc3::Handle) {return current;}
wc3::Bits __cdecl X(wc3::Handle) {float f=100;wc3::Bits b;memcpy(&b,&f,4);return b;}
wc3::Bits __cdecl Y(wc3::Handle) {float f=200;wc3::Bits b;memcpy(&b,&f,4);return b;}
BOOL __cdecl Stop(wc3::Handle,int) {++stops;current=0;return TRUE;}
BOOL __cdecl Use(wc3::Handle,wc3::Handle) {if(observing)assert(observing->Active());++uses;if(allowed)--charges;return allowed;}
BOOL __cdecl Point(wc3::Handle,wc3::Handle,float* x,float* y) {++points;px=*x;py=*y;return allowed;}
BOOL __cdecl Target(wc3::Handle,wc3::Handle,wc3::Handle t) {++targets;target=t;return allowed;}
BOOL __cdecl Drop(wc3::Handle,wc3::Handle,float* x,float* y) {++drops;px=*x;py=*y;return allowed;}
BOOL __cdecl Move(wc3::Handle,wc3::Handle item,int destination) {++moves;for(int i=0;i<6;++i)if(items[i]==item) {std::swap(items[i],items[destination]);break;}return allowed;}
}
namespace NativeItemCommands {unsigned Forms(uintptr_t,wc3::Handle,int,wc3::Handle) {return forms;}}
namespace wc3 {
void Log(const char*,...) {}
float Real(Bits bits) {float f;memcpy(&f,&bits,4);return f;}
int (__cdecl* GetUnitTypeId)(Handle)=Type;
int (__cdecl* UnitInventorySize)(Handle)=Size;
Handle (__cdecl* UnitItemInSlot)(Handle,int)=Slot;
int (__cdecl* GetUnitCurrentOrder)(Handle)=Order;
BOOL (__cdecl* IssueImmediateOrderById)(Handle,int)=Stop;
Bits (__cdecl* GetUnitX)(Handle)=X;
Bits (__cdecl* GetUnitY)(Handle)=Y;
BOOL (__cdecl* UnitUseItem)(Handle,Handle)=Use;
BOOL (__cdecl* UnitUseItemPoint)(Handle,Handle,float*,float*)=Point;
BOOL (__cdecl* UnitUseItemTarget)(Handle,Handle,Handle)=Target;
BOOL (__cdecl* UnitDropItemPoint)(Handle,Handle,float*,float*)=Drop;
BOOL (__cdecl* UnitDropItemSlot)(Handle,Handle,int)=Move;
}
int main() {
    InventoryControls controls;using Kind=InventoryControls::Kind;
    controls.Queue({Kind::Numbers});assert(controls.RoutingNumbers() && !controls.Numbers());
    controls.Queue({Kind::Panel});assert(controls.RoutingOpen() && !controls.Open());controls.Reset();assert(!controls.RoutingOpen());
    controls.TogglePanel();controls.Down(0,11,20,20);auto command=controls.Up(0,21,20,4);
    assert(command.kind==InventoryControls::Command::Use && command.item==11);
    controls.Down(0,11,20,20);controls.Motion(40,20,4);command=controls.Up(-1,40,20,4);
    assert(command.kind==InventoryControls::Command::Drop && command.item==11);
    controls.Down(0,11,20,20);controls.Motion(40,20,4);command=controls.Up(0,20,20,4);assert(command.kind==InventoryControls::Command::None);
    controls.Down(0,11,20,20);command=controls.Up(3,100,20,4);assert(command.kind==InventoryControls::Command::Move && command.destination==3);
    controls.Down(0,11,20,20);controls.Cancel();assert(controls.Up(-1,40,20,4).kind==InventoryControls::Command::None);
    controls.Down(0,11,20,20);assert(controls.Up(-2,40,20,4).kind==InventoryControls::Command::None);
    for(int key='0';key<='9';++key) {
        assert(InventoryControls::NumberSlot(key,false,false)==-1 && InventoryControls::NumberSlot(key,true,true)==-1);
        assert(InventoryControls::NumberSlot(key,true,false)==(key>='1' && key<='6' ? key-'1' : -1));
    }
    auto layout=InventoryHudLayout::Calculate(1280,720,7,true,true);
    for(int i=0;i<6;++i)assert(layout.Slot(layout.x+i*(layout.side+layout.gap),layout.y)==i);
    assert(layout.Slot(layout.x+layout.side,layout.y)==-1 && layout.Slot(layout.x,layout.y+layout.side)==-1);
    assert(layout.PointerSlot(layout.x+layout.side,layout.y)==-2 && layout.PointerSlot(20,20)==-1);
    // Projection/unprojection round trips catch mirrored cursor targets and aspect/FOV errors.
    for(float yaw:{0.f,90.f,270.f})for(float pitch:{-20.f,0.f,40.f})for(float x:{100.f,640.f,1180.f}) {
        auto ray=CursorAimRay::Direction(yaw,pitch,55,x,400,1280,720);
        UnitHealthBar unit;unit.health=unit.maximum=100;for(int i=0;i<3;++i)unit.position[i]=ray[i]*500;
        WorldLabelView view;view.active=true;view.yaw=yaw;view.pitch=pitch;view.verticalFov=55;view.aspect=1280.f/720;
        HealthBarPlacement p;assert(HealthBarView::Project(unit,view,1280,720,8,5000,p));
        assert(std::abs(p.x-x)<.01f && std::abs(p.y-400)<.01f);
    }
    NativeItemActions actions;observing=&actions;using Result=NativeItemActions::Result;
    assert(actions.Use(1,0,11,nullptr)==Result::Issued && uses==1 && charges==2);
    allowed=false;assert(actions.Use(1,0,11,nullptr)==Result::Rejected && charges==2 && !actions.Active());allowed=true;
    forms=0;assert(actions.Use(1,0,11,nullptr)==Result::Rejected && uses==2);
    forms=2;assert(actions.Use(1,0,11,nullptr)==Result::TargetRequired && points==0);
    SpellAim aim;aim.target=777;aim.point=true;aim.x=600;aim.y=400;
    assert(actions.Use(1,0,11,&aim)==Result::Issued && points==1 && targets==0 && px==600 && py==400);
    forms=4;assert(actions.Use(1,0,11,&aim,true)==Result::Issued && target==1);
    // Native targeted item wrappers may queue successfully yet return false until the next simulation tick.
    allowed=false;assert(actions.Use(1,0,11,&aim,true)==Result::Requested && actions.Active());
    current=852008;Sleep(110);actions.Update();assert(actions.Active());current=0;actions.Update();assert(!actions.Active());allowed=true;
    aim.target=0;assert(actions.Use(1,0,11,&aim)==Result::Rejected);
    forms=2;aim.point=false;assert(actions.Use(1,0,11,&aim)==Result::Rejected);
    assert(actions.Drop(1,0,11,90,80) && drops==1 && std::abs(px-100)<.01f && std::abs(py-280)<.01f);
    assert(!actions.Drop(1,0,99,0,80) && drops==1);
    assert(actions.Move(1,0,11,5) && moves==1 && items[5]==11 && items[0]==66);
    assert(actions.Use(1,0,11,nullptr)==Result::Rejected); // The pressed item was replaced in its old slot.
    actions.Cancel();observing=nullptr;assert(!actions.Active());
    // The real coordinator consumes short event sequences once, cancels stale gestures and clears UI on disable.
    InventoryController controller;GameplaySettings settings;int prepares=0;forms=1;
    auto pick=[&](int,int,int,int){return aim;};auto prepare=[&] {++prepares;controller.CancelCasting();};
    controller.Input({Kind::Numbers});InventoryControls::Event use{Kind::Use};use.slot=5;controller.Input(use);
    int before=uses;controller.Update(1,settings,false,0,false,pick,prepare);assert(controller.State().Numbers() && uses==before+1);
    controller.Update(1,settings,false,0,false,pick,prepare);assert(uses==before+1);
    controller.Input({Kind::Panel});controller.Update(1,settings,false,0,false,pick,prepare);assert(controller.State().Open());
    InventoryControls::Event down{Kind::Down};down.width=1280;down.height=720;down.x=layout.x+layout.side/2;down.y=layout.y+layout.side/2;
    controller.Input(down);controller.Update(1,settings,false,0,false,pick,prepare);assert(controller.State().PressedItem()==66);
    items[0]=99;auto up=down;up.kind=Kind::Up;up.x=20;up.y=20;controller.Input(up);
    controller.Update(1,settings,false,0,false,pick,prepare);assert(drops==1 && controller.State().PressedSlot()==-1);
    settings.inventoryControls=false;controller.Update(1,settings,false,0,false,pick,prepare);assert(!controller.State().Open() && !controller.State().Numbers());
    controller.Reset();assert(!controller.Active() && prepares>=2);
    // An interrupted drag must release Win32 capture even if F8/view exit happens before button-up.
    HWND window=CreateWindowA("STATIC","",WS_POPUP,0,0,10,10,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);assert(window);
    settings.inventoryControls=true;controller.Input({Kind::Panel});controller.Update(1,settings,false,0,false,pick,prepare);
    SetCapture(window);assert(GetCapture()==window);controller.ClearInput();assert(!GetCapture());
    controller.Input({Kind::Panel});SetCapture(window);controller.Reset();assert(!GetCapture());DestroyWindow(window);
    // Real startup/F8 settings enable item numbers without Tab; gesture cleanup preserves a later Tab choice.
    controller.ApplySettings(settings);assert(controller.State().Numbers());
    controller.Input({Kind::Numbers});controller.Update(1,settings,false,0,false,pick,prepare);
    assert(!controller.State().Numbers());controller.ClearInput();assert(!controller.State().Numbers());
    controller.Reset();assert(controller.State().Numbers());
    settings.inventoryNumbersDefault=false;controller.ApplySettings(settings);assert(!controller.State().Numbers());
    controller.Input({Kind::Numbers});controller.Update(1,settings,false,0,false,pick,prepare);assert(controller.State().Numbers());
    controller.ApplySettings(settings);assert(!controller.State().Numbers());
    settings.inventoryControls=false;settings.inventoryNumbersDefault=true;controller.ApplySettings(settings);controller.Reset();assert(!controller.State().Numbers());
}
