#pragma once
#include <windows.h>
#include <optional>
class ShooterController;

// Translate window messages to game-thread mailboxes; forwarding remains the host's responsibility.
class InputDispatcher {
public:
    explicit InputDispatcher(ShooterController& controller) : controller_(controller) {}
    std::optional<LRESULT> Handle(HWND window, UINT message, WPARAM key, LPARAM data, bool healthy);
private:
    ShooterController& controller_;
};
