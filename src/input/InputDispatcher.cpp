#include "InputDispatcher.hpp"
#include "../application/ShooterController.hpp"

std::optional<LRESULT> InputDispatcher::Handle(HWND window, UINT message, WPARAM key, LPARAM data, bool healthy) {
    if (message==WM_INPUT) {
        // Raw packets retain fast movement and keep 360-degree look independent of cursor recentering.
        controller_.MouseInput(data,healthy && controller_.Visible() && !controller_.Buying() && GetForegroundWindow()==window);
        if (healthy && controller_.Visible()) return DefWindowProcA(window,message,key,data);
    }
    // Focus transitions discard queued relative input; renderer recovery belongs to the runtime.
    if (message == WM_ACTIVATEAPP) {
        controller_.ResetMouse();
    }
    // F6 belongs to the extension even before FPS is active, avoiding native quicksave.
    if (key == VK_F6 && (message == WM_KEYDOWN || message == WM_KEYUP)) {
        if (message == WM_KEYDOWN && !(data & (1L << 30))) controller_.RequestToggle();
        return 0;
    }
    // Consume FPS inputs so Warcraft does not also issue RTS orders or select units.
    if (healthy && controller_.Visible()) {
        // B/period and menu rows are mailbox events; no native economy calls occur in the input handler.
        if ((message==WM_KEYDOWN || message==WM_KEYUP) && (key=='B' || key==VK_OEM_PERIOD ||
            (controller_.Buying() && key>='0' && key<='9'))) {
            if (message==WM_KEYDOWN && !(data&(1L<<30))) {
                if (key=='B') controller_.RequestBuyToggle();
                else if (key==VK_OEM_PERIOD) controller_.RequestBuyAmmo();
                else controller_.RequestBuyKey(int(key-'0'));
            }
            return 0;
        }
        if (controller_.Buying() && message==WM_LBUTTONDOWN) {
            RECT client={};GetClientRect(window,&client);
            controller_.RequestBuyClick(short(LOWORD(data)),short(HIWORD(data)),client.right,client.bottom);return 0;
        }
        // Queue each signed wheel packet for the game tick; Warcraft must not also zoom its RTS camera.
        if (message == WM_MOUSEWHEEL) {
            if (GetForegroundWindow() == window) controller_.RequestWeaponWheel(GET_WHEEL_DELTA_WPARAM(key));
            return 0;
        }
        // Free refill/grant keys are consumed before Warcraft interprets native strategy shortcuts.
        if ((key == VK_F7 || key == VK_F9) && (message == WM_KEYDOWN || message == WM_KEYUP)) {
            if (message == WM_KEYDOWN && !(data & (1L << 30))) {
                if (key==VK_F9) controller_.RequestAllWeapons();else controller_.RequestRefill();
            }
            return 0;
        }
        // Preserve short pickup/squad/config taps; J releases followers without leaving FPS.
        if ((key=='E' || key=='H' || key=='O' || key=='J' || key==VK_F8) && (message==WM_KEYDOWN || message==WM_KEYUP)) {
            if (message==WM_KEYDOWN && !(data&(1L<<30))) {
                if (key=='E') controller_.RequestItemPickup();
                else if (key=='H' || key=='O') controller_.RequestSquad(key=='O');
                else if (key=='J') controller_.RequestSquadRelease();
                else controller_.RequestSettingsReload();
            }
            return 0;
        }
        // The native pause menu requires restored RTS input before handling its hotkey.
        // Windows delivers F10 as a system key even when Alt is not pressed.
        if (key == VK_F10 && (message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)) {
            if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && !(data & (1L << 30))) controller_.RequestMenu();
            return 0;
        }
        if ((message == WM_KEYDOWN || message == WM_KEYUP) &&
            (key == 'W' || key == 'A' || key == 'S' || key == 'D' || key == 'R' ||
             key == VK_F6 || key == VK_SPACE || key == VK_CONTROL || key == VK_SHIFT || (key >= '1' && key <= '1' + WeaponSlots::Count - 1))) return 0;
        if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONDOWN ||
            message == WM_RBUTTONUP || message == WM_MOUSEMOVE) return 0;
        // Native HUD stays enabled for portrait rendering; unused keys must not issue RTS orders in FPS.
        if (message==WM_KEYDOWN || message==WM_KEYUP || message==WM_CHAR) return 0;
    }
    return std::nullopt;
}
