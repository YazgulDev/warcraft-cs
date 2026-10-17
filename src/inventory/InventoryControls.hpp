#pragma once
#include <cstdint>
#include <deque>
#include <cmath>

// Gesture state stores the exact pressed item token, so a changed slot cannot use/drop its replacement.
class InventoryControls {
public:
    enum class Kind {Numbers,Panel,Close,Cancel,Down,Up,Move,Use};
    struct Event {Kind kind;int x=0,y=0,width=0,height=0,slot=-1;bool self=false;};
    struct Command {enum Kind {None,Use,Drop,Move} kind=None;int slot=-1,destination=-1;uint32_t item=0;};
    void Queue(Event e) {
        if(e.kind==Kind::Move && !events_.empty() && events_.back().kind==Kind::Move) {events_.back()=e;return;}
        if(events_.size()>=64) {events_.clear();events_.push_back({Kind::Cancel});return;}
        events_.push_back(e);
    }
    bool Next(Event& e) {if(events_.empty())return false;e=events_.front();events_.pop_front();return true;}
    // Gesture cleanup can preserve Tab; startup/map reload supplies its configured default.
    void Reset(bool numbers=false) {numbers_=numbers;open_=false;Cancel();events_.clear();}
    void ToggleNumbers() {numbers_=!numbers_;}
    void TogglePanel() {open_=!open_;Cancel();}
    void Close() {open_=false;Cancel();}
    void Cancel() {pressed_=-1;item_=0;target_=-1;targetItem_=0;dragged_=false;}
    bool Numbers() const {return numbers_;}
    bool Open() const {return open_;}
    // Route short Tab/I + click/number sequences consistently even before the next simulation tick.
    bool RoutingOpen() const {bool open=open_;for(auto e:events_) {if(e.kind==Kind::Panel)open=!open;else if(e.kind==Kind::Close)open=false;}return open;}
    bool RoutingNumbers() const {bool numbers=numbers_;for(auto e:events_)if(e.kind==Kind::Numbers)numbers=!numbers;return numbers;}
    int PressedSlot() const {return pressed_;}
    uint32_t PressedItem() const {return item_;}
    bool Dragging() const {return pressed_>=0 && dragged_;}
    int TargetSlot() const {return target_;}
    uint32_t TargetItem() const {return targetItem_;}
    void Target(int slot,uint32_t item) {Cancel();target_=slot;targetItem_=item;}
    void Down(int slot,uint32_t item,int x,int y) {Cancel();if(slot>=0 && item) {pressed_=slot;item_=item;startX_=x;startY_=y;}}
    void Motion(int x,int y,int threshold) {
        if(pressed_>=0 && (std::abs(x-startX_)>=threshold || std::abs(y-startY_)>=threshold))dragged_=true;
    }
    Command Up(int slot,int x,int y,int threshold) {
        Motion(x,y,threshold);Command result;
        if(pressed_>=0) {
            result.slot=pressed_;result.item=item_;result.destination=slot;
            if(dragged_ && slot==-1)result.kind=Command::Drop;
            else if(dragged_ && slot>=0 && slot!=pressed_)result.kind=Command::Move;
            else if(!dragged_ && slot==pressed_)result.kind=Command::Use;
        }
        Cancel();return result;
    }
    static int NumberSlot(int key,bool enabled,bool buying) {return enabled && !buying && key>='1' && key<='6' ? key-'1' : -1;}
private:
    bool numbers_=false,open_=false,dragged_=false;
    int pressed_=-1,target_=-1,startX_=0,startY_=0;
    uint32_t item_=0,targetItem_=0;
    std::deque<Event> events_;
};
