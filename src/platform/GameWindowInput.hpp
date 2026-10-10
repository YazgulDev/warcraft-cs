#pragma once
#include <windows.h>
#include <map>

// Keep a forwarding procedure per live render window across movie/window transitions.
class GameWindowInput {
public:
    explicit GameWindowInput(WNDPROC callback):callback_(callback) {}
    bool Observe(HWND window);
    WNDPROC Previous(HWND window) const;
    void Destroyed(HWND window);
    HWND Window() const { return window_; }
private:
    WNDPROC callback_;
    HWND window_=nullptr;
    std::map<HWND,WNDPROC> previous_;
};
