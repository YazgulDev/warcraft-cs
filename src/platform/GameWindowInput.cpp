#include "GameWindowInput.hpp"
#include "WarcraftApi.hpp"

bool GameWindowInput::Observe(HWND window) {
    DWORD process=0;
    if (!window || !IsWindow(window) || GetWindowThreadProcessId(window,&process)!=GetCurrentThreadId() ||
        process!=GetCurrentProcessId()) return false;
    auto current=reinterpret_cast<WNDPROC>(GetWindowLongPtrA(window,GWLP_WNDPROC));
    auto found=previous_.find(window);
    bool changed=window_!=window;
    if (found==previous_.end() || current==found->second) {
        // Movie playback can restore Warcraft's original procedure without destroying its window.
        // Reinstall only over that known procedure; wrapping a third-party subclass could form a loop.
        if (current==callback_) return false;
        SetLastError(0);
        auto previous=reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(callback_)));
        if (!previous && GetLastError()) {wc3::LogError("Input window attach failed hwnd=%p error=%lu",window,GetLastError());return false;}
        bool recovered=found!=previous_.end();
        previous_[window]=previous;changed=true;
        wc3::Log("Input window %s hwnd=%p previous=%p",recovered ? "recovered after native reset" : "attached",window,previous);
    }
    // Retain old live-window forwarding records: late destroy messages may arrive after replacement.
    window_=window;
    return changed;
}
WNDPROC GameWindowInput::Previous(HWND window) const {
    auto found=previous_.find(window);return found==previous_.end() ? nullptr : found->second;
}
void GameWindowInput::Destroyed(HWND window) {
    previous_.erase(window);
    if (window_==window) window_=nullptr;
    wc3::Log("Input window destroyed hwnd=%p",window);
}
