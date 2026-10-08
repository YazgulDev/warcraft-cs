#pragma once

// CS-style number-key categories; presentation and native payment remain separate from navigation.
class BuyMenu {
public:
    enum class Page { Closed, Main, Pistols, Rifles, Melee };
    struct Action { int slot=-1;bool ammo=false;bool allAmmo=false; };
    Page Current() const { return page_; }
    bool Open() const { return page_!=Page::Closed; }
    void Close() { page_=Page::Closed; }
    void Toggle() { page_=Open() ? Page::Closed : Page::Main; }
    Action Select(int key,int current) {
        Action result;
        if (!Open()) return result;
        if (key==0) { page_=page_==Page::Main ? Page::Closed : Page::Main;return result; }
        if (page_==Page::Main) {
            if (key==1) page_=Page::Pistols;
            if (key==2) page_=Page::Rifles;
            if (key==3) page_=Page::Melee;
            if (key==4) result.slot=5;
            if (key==5) { result.slot=current;result.ammo=true; }
            if (key==6) result.allAmmo=true;
        } else if (page_==Page::Pistols && key==1) result.slot=2;
        else if (page_==Page::Rifles && key>=1 && key<=3) result.slot=key==3 ? 3 : key-1;
        else if (page_==Page::Melee && key>=1 && key<=2) result.slot=key==1 ? 4 : 6;
        return result;
    }
private:
    Page page_=Page::Closed;
};
