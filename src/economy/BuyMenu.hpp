#pragma once
#include "BuyCatalog.hpp"

// Preserve original CS category numbers while omitting categories we do not implement.
class BuyMenu {
public:
    enum class Page { Closed, Main, Pistols, Rifles, Equipment, Shotguns, SMGs, Machineguns, HalfLife };
    // Currency buttons are available on every page without renumbering weapon/category shortcuts.
    static constexpr int PayDollars=10,PayGold=11;
    struct Action { int slot=-1;bool primaryAmmo=false;bool secondaryAmmo=false;int currency=-1; };
    Page Current() const { return page_; }
    bool Open() const { return page_!=Page::Closed; }
    void Close() { page_=Page::Closed; }
    void Toggle() { page_=Open() ? Page::Closed : Page::Main; }
    static BuyCatalog::Category Category(Page page) {
        if(page==Page::HalfLife)return BuyCatalog::Category::HalfLife;
        if(page==Page::Shotguns)return BuyCatalog::Category::Shotguns;
        if(page==Page::SMGs)return BuyCatalog::Category::SMGs;
        if(page==Page::Machineguns)return BuyCatalog::Category::Machineguns;
        return page==Page::Pistols ? BuyCatalog::Category::Pistols : page==Page::Rifles ?
            BuyCatalog::Category::Rifles : page==Page::Equipment ? BuyCatalog::Category::Equipment : BuyCatalog::Category::Main;
    }
    Action Select(int key,int /*current*/) {
        Action result;
        if (!Open()) return result;
        if(key==PayDollars || key==PayGold){result.currency=key==PayDollars ? 0 : 1;return result;}
        if (key==0) { page_=page_==Page::Main ? Page::Closed : Page::Main;return result; }
        if (page_==Page::Main) {
            if (key==1) page_=Page::Pistols;
            if (key==2) page_=Page::Shotguns;
            if (key==3) page_=Page::SMGs;
            if (key==5) page_=Page::Machineguns;
            if (key==4) page_=Page::Rifles;
            if (key==6) result.primaryAmmo=true;
            if (key==7) result.secondaryAmmo=true;
            if (key==8) page_=Page::Equipment;
            if (key==9) page_=Page::HalfLife; // Preserve every existing CS purchase key.
        } else if (auto item=BuyCatalog::Find(Category(page_),key)) result.slot=item->slot;
        return result;
    }
private:
    Page page_=Page::Closed;
};
