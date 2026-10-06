#include "MouseLook.hpp"

void MouseLook::Attach(HWND window) {
    window_=window;
    // Keep legacy mouse messages available to Warcraft's RTS UI; FPS consumes only its own raw deltas.
    RAWINPUTDEVICE device={0x01,0x02,0,window};
    registered_=RegisterRawInputDevices(&device,1,sizeof(device))!=FALSE;
    wc3::Log("Mouse look raw input registered=%d error=%lu",registered_,registered_ ? 0UL : GetLastError());
    Reset();
}
void MouseLook::Reset() { dx_=dy_=0;captured_=false; }
void MouseLook::Input(LPARAM packet,bool accept) {
    RAWINPUT input={};UINT size=sizeof(input);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(packet),RID_INPUT,&input,&size,sizeof(RAWINPUTHEADER))==UINT(-1) ||
        input.header.dwType!=RIM_TYPEMOUSE) return;
    // Absolute tablet packets use the cursor fallback; a relative mouse uses unaccelerated native counts.
    absolute_=(input.data.mouse.usFlags&MOUSE_MOVE_ABSOLUTE)!=0;
    if (!accept || absolute_) { dx_=dy_=0;return; }
    dx_+=input.data.mouse.lLastX;dy_+=input.data.mouse.lLastY;
}
void MouseLook::Sample(float& dx,float& dy) {
    dx=dy=0;
    if (!window_ || GetForegroundWindow()!=window_) { Reset();return; }
    RECT client={};POINT center={},cursor={};
    GetClientRect(window_,&client);center.x=(client.right-client.left)/2;center.y=(client.bottom-client.top)/2;
    ClientToScreen(window_,&center);
    if (captured_) {
        if (Raw()) { dx=float(dx_);dy=float(dy_); }
        else { GetCursorPos(&cursor);dx=float(cursor.x-center.x);dy=float(cursor.y-center.y); }
    }
    // Only activation/focus transitions discard stale cursor position; screen edges never limit raw turns.
    dx_=dy_=0;captured_=true;SetCursorPos(center.x,center.y);
}
