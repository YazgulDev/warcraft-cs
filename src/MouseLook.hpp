#pragma once
#include "WarcraftApi.hpp"

// Own relative mouse packets and cursor capture; gameplay only receives a displacement per frame.
class MouseLook {
public:
    void Attach(HWND window);
    void Input(LPARAM packet,bool accept);
    void Reset();
    void Sample(float& dx,float& dy);
    bool Raw() const { return registered_ && !absolute_; }
private:
    HWND window_=nullptr;
    bool registered_=false,absolute_=false,captured_=false;
    double dx_=0,dy_=0;
};
