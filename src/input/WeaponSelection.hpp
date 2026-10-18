#pragma once
#include "../economy/BuyCatalog.hpp"
#include <array>
#include <vector>

// A pending choice is separate from the equipped weapon until an explicit confirmation.
class WeaponSelection {
public:
    using Available=std::array<bool,WeaponSlots::Count>;
    static int Group(int slot) {
        const auto* item=BuyCatalog::Slot(slot);
        if(!item)return 0;
        if(WeaponSlots::Melee(slot))return 3;
        if(WeaponSlots::Grenade(slot))return 4;
        if(slot==WeaponSlots::C4)return 5;
        return item->category==BuyCatalog::Category::Pistols ? 2 : 1;
    }
    static const char* Label(int group) {
        static const char* labels[]={"","PRIMARY","PISTOLS","MELEE","GRENADES","C4"};
        return group>=1 && group<=5 ? labels[group] : "";
    }
    static std::vector<int> Items(int group,const Available& available) {
        std::vector<int> items;
        // The purchase catalog remains the single registration list for all supported weapons.
        for(const auto& item:BuyCatalog::Entries)if(available[item.slot] && Group(item.slot)==group)items.push_back(item.slot);
        return items;
    }
    bool Open() const {return pending_>=0;}
    int Pending() const {return pending_;}
    void Close() {pending_=-1;}
    bool Category(int group,int equipped,const Available& available,double now) {
        auto items=Items(group,available);if(items.empty())return false;
        int selected=items.front();
        for(size_t i=0;i<items.size();++i) {
            if(Open() && Group(pending_)==group && items[i]==pending_) {selected=items[(i+1)%items.size()];break;}
            if(!Open() && items[i]==equipped)selected=equipped;
        }
        pending_=selected;touched_=now;return true;
    }
    void Cycle(long long steps,int equipped,const Available& available,double now) {
        if(!steps)return;
        Available all;all.fill(true);std::vector<int> order;
        // Follow the displayed groups and their row order, rather than unrelated persistent slot IDs.
        for(int group=1;group<=5;++group) {
            auto items=Items(group,all);order.insert(order.end(),items.begin(),items.end());
        }
        int base=Open() ? pending_ : equipped,index=-1,count=0;
        for(size_t i=0;i<order.size();++i) {
            if(order[i]==base)index=int(i);
            if(available[order[i]])++count;
        }
        if(!count){Close();return;}
        int direction=steps<0 ? -1 : 1;
        if(index<0)index=direction>0 ? int(order.size())-1 : 0;
        // Reduce complete laps by the actual available count. Keep the first move even when the
        // equipped consumable is depleted, so its position still anchors the next/previous choice.
        int moves=1+int((steps>0 ? steps-1 : -(steps+1))%count);
        for(int move=0;move<moves;++move)do {
            index=(index+direction+int(order.size()))%int(order.size());
        } while(!available[order[index]]);
        pending_=order[index];touched_=now;
    }
    void Expire(double now,double seconds,const Available& available) {
        // Focus/time changes and inventory depletion must never leave a stale selectable consumable.
        if(Open() && (!available[pending_] || now<touched_ || now-touched_>=seconds))Close();
    }
    int Confirm(const Available& available) {
        int result=Open() && available[pending_] ? pending_ : -1;Close();return result;
    }
private:
    int pending_=-1;
    double touched_=0;
};
