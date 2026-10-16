#include "InputDispatcher.hpp"
#ifdef WCS_GAMEPLAY_TEST
#include "../../tests/BackgroundGameplay.hpp"
#endif
#include "../application/ShooterController.hpp"

std::optional<LRESULT> InputDispatcher::Handle(HWND window,UINT message,WPARAM key,LPARAM data,bool healthy) {
#ifdef WCS_GAMEPLAY_TEST
    // A marked isolated loading-screen run may dismiss Warcraft's press-any-key gate before the world exists.
    char loadKey[2]={};
    if(BackgroundGameplay::Enabled() && !controller_.Visible() && key==VK_SPACE &&
        (message==WM_KEYDOWN || message==WM_KEYUP || message==WM_CHAR) &&
        GetEnvironmentVariableA("WCS_LOAD_KEY",loadKey,2)==1 && loadKey[0]=='1')return std::nullopt;
    // A background oracle otherwise accepts only its file mailbox, even when its window becomes active.
    if(BackgroundGameplay::Enabled() && (message==WM_INPUT ||
        (message>=WM_KEYFIRST && message<=WM_KEYLAST) || (message>=WM_MOUSEFIRST && message<=WM_MOUSELAST)))
        return message==WM_INPUT ? DefWindowProcA(window,message,key,data) : 0;
#endif
    const auto& bindings=controller_.Settings().shortcuts;
    auto match=[&](ShortcutAction action) {return bindings.Matches(action,int(key));};
    bool down=message==WM_KEYDOWN || message==WM_SYSKEYDOWN;
    bool up=message==WM_KEYUP || message==WM_SYSKEYUP;
    bool tap=down && !(data&(1L<<30));
    // Log only configured mod controls, never text entry or unrelated desktop keystrokes.
    bool control=key==VK_F6 || match(ShortcutAction::ToggleFPS) || match(ShortcutAction::ToggleThirdPerson) || match(ShortcutAction::ToggleUnitView);
    if(healthy && controller_.Visible())for(size_t i=0;i<ShortcutBindings::Count;++i)control|=match(ShortcutAction(i));
    if(control && (up || tap))wc3::Trace("control key=%u event=%s fps=%d buying=%d healthy=%d",unsigned(key),up ? "up" : "down",controller_.Visible(),controller_.Buying(),healthy);
    if(healthy && controller_.Visible() && (message==WM_LBUTTONDOWN || message==WM_LBUTTONUP || message==WM_RBUTTONDOWN || message==WM_RBUTTONUP || message==WM_MOUSEWHEEL))
        wc3::Trace("control mouseMessage=%u wheel=%d buying=%d",message,message==WM_MOUSEWHEEL ? GET_WHEEL_DELTA_WPARAM(key) : 0,controller_.Buying());
    if(message==WM_INPUT) {
        controller_.MouseInput(data,healthy && controller_.Visible() && !controller_.CursorMode() && GetForegroundWindow()==window);
        if(healthy && controller_.Visible())return DefWindowProcA(window,message,key,data);
    }
    if(message==WM_ACTIVATEAPP) {
        controller_.ResetMouse();
        if(!key) {controller_.RequestInventory({InventoryControls::Kind::Close});if(GetCapture()==window)ReleaseCapture();}
    }
    // F6 remains a recovery alias even after remapping ToggleFPS, including native campaign movie transitions.
    if((key==VK_F6 || match(ShortcutAction::ToggleFPS)) && (down || up)) {
        if(tap)controller_.RequestToggle();return 0;
    }
    // The dedicated third-person entry works from RTS too; only the game-thread mailbox changes modes.
    if(healthy && match(ShortcutAction::ToggleThirdPerson) && (down || up)) {
        if(tap)controller_.RequestThirdPersonToggle();return 0;
    }
    // F9 works directly from RTS and restores RTS on its second press, independently of F6/F7.
    if(healthy && match(ShortcutAction::ToggleUnitView) && (down || up)) {
        if(tap)controller_.RequestUnitViewToggle();return 0;
    }
    if(!healthy || !controller_.Visible())return std::nullopt;
    // Native pause is always available and regains ordinary RTS input before its system key is forwarded.
    if(key==VK_F10 && (down || up)) {if(tap)controller_.RequestMenu();return 0;}
    if(controller_.UnitFirstPerson()) {
        // Only view recovery, settings and squad control are actions here; held WASD/jump are sampled by physics.
        if(tap) {
            if(match(ShortcutAction::ReloadSettings))controller_.RequestSettingsReload();
            else if(match(ShortcutAction::ToggleView))controller_.RequestViewToggle();
            else if(match(ShortcutAction::SquadFight))controller_.RequestSquad(false);
            else if(match(ShortcutAction::SquadFollow))controller_.RequestSquad(true);
            else if(match(ShortcutAction::SquadRelease))controller_.RequestSquadRelease();
        }
        if(down || up || message==WM_CHAR || message==WM_SYSCHAR || (message>=WM_MOUSEFIRST && message<=WM_MOUSELAST))return 0;
    }
    // Inventory gestures enter a bounded game-thread mailbox; Windows events never mutate native items.
    if(controller_.Settings().inventoryControls) {
        if((match(ShortcutAction::InventoryNumbers) || match(ShortcutAction::InventoryPanel)) && (down || up)) {
            if(tap)controller_.RequestInventory({match(ShortcutAction::InventoryNumbers) ? InventoryControls::Kind::Numbers : InventoryControls::Kind::Panel});
            if(GetCapture()==window)ReleaseCapture();return 0;
        }
        int slot=InventoryControls::NumberSlot(int(key),controller_.ItemControls().RoutingNumbers() || controller_.InventoryInputOpen(),controller_.Buying());
        if(slot>=0 && (down || up)) {
            if(tap) {InventoryControls::Event e{InventoryControls::Kind::Use};e.slot=slot;e.self=(GetAsyncKeyState(bindings.Key(ShortcutAction::SelfCast))&0x8000)!=0;controller_.RequestInventory(e);}return 0;
        }
    }
    if(controller_.InventoryInputOpen()) {
        if(tap && key==VK_ESCAPE) {controller_.RequestInventoryClose();if(GetCapture()==window)ReleaseCapture();return 0;}
        if(tap && match(ShortcutAction::ReloadSettings)) {controller_.RequestSettingsReload();return 0;}
        if(message==WM_LBUTTONDOWN || message==WM_LBUTTONUP || message==WM_MOUSEMOVE || message==WM_RBUTTONDOWN) {
            RECT client={};GetClientRect(window,&client);
            InventoryControls::Event e{message==WM_LBUTTONDOWN ? InventoryControls::Kind::Down :
                message==WM_LBUTTONUP ? InventoryControls::Kind::Up : message==WM_RBUTTONDOWN ? InventoryControls::Kind::Cancel : InventoryControls::Kind::Move};
            e.x=short(LOWORD(data));e.y=short(HIWORD(data));e.width=client.right;e.height=client.bottom;
            e.self=(GetAsyncKeyState(bindings.Key(ShortcutAction::SelfCast))&0x8000)!=0; // Preserve Alt on short click sequences before the next game tick.
            controller_.RequestInventory(e);
            if(message==WM_LBUTTONDOWN)SetCapture(window);
            if((message==WM_LBUTTONUP || message==WM_RBUTTONDOWN) && GetCapture()==window)ReleaseCapture();
        }
        // Clicking/dragging never leaks through to native RTS orders, scope, firing or camera motion.
        if(down || up || message==WM_CHAR || message==WM_SYSCHAR || (message>=WM_MOUSEFIRST && message<=WM_MOUSELAST))return 0;
    }
    if(controller_.Buying() && message==WM_LBUTTONDOWN) {
        RECT client={};GetClientRect(window,&client);controller_.RequestBuyClick(short(LOWORD(data)),short(HIWORD(data)),client.right,client.bottom);return 0;
    }
    if(message==WM_MOUSEWHEEL) {
        // The controller routes TPS zoom separately from FPS weapon selection on its game-thread mailbox.
        if(GetForegroundWindow()==window)controller_.RequestMouseWheel(GET_WHEEL_DELTA_WPARAM(key));return 0;
    }
    // New deliberate movement/fire interrupts channeling; a held key cannot repeatedly cancel new casts.
    if(message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN)controller_.RequestSpellCancel();
    if(down || up) {
        if(tap && !controller_.Buying()) {
            for(auto action:{ShortcutAction::Forward,ShortcutAction::Backward,ShortcutAction::Left,ShortcutAction::Right,
                ShortcutAction::Jump,ShortcutAction::Crouch,ShortcutAction::Reload})if(match(action))controller_.RequestSpellCancel();
            for(int i=0;i<WeaponSlots::Count;++i)if(match(ShortcutAction(int(ShortcutAction::Weapon1)+i)))controller_.RequestSpellCancel();
        }
        // Buy row numbers belong to the open catalog even when weapon shortcuts were remapped.
        if(controller_.Buying() && key>='0' && key<='9') {if(tap)controller_.RequestBuyKey(int(key-'0'));return 0;}
        if(tap) {
            // A queued view switch cannot alter native rendering/input from inside WindowProc.
            if(match(ShortcutAction::ToggleView))controller_.RequestViewToggle();
            else if(match(ShortcutAction::Buy))controller_.RequestBuyToggle();
            else if(match(ShortcutAction::BuyAmmo))controller_.RequestBuyAmmo();
            // Minimap follows the same configurable, one-tap input routing as other HUD switches.
            else if(match(ShortcutAction::Minimap))controller_.RequestMinimapToggle();
            else if(match(ShortcutAction::Speed))controller_.RequestSpeedToggle();
            else if(match(ShortcutAction::Fog))controller_.RequestFogToggle();
            else if(match(ShortcutAction::HealthBars))controller_.RequestHealthBarToggle();
            // One edge queues the shared FPS/TPS allied-contact preference.
            else if(match(ShortcutAction::ToggleAllyTargeting))controller_.RequestAllyTargetToggle();
            else if(match(ShortcutAction::Refill))controller_.RequestRefill();
            else if(match(ShortcutAction::AllWeapons))controller_.RequestAllWeapons();
            else if(match(ShortcutAction::ReloadSettings))controller_.RequestSettingsReload();
            else if(!controller_.Buying()) {
                if(match(ShortcutAction::Pickup))controller_.RequestItemPickup();
                else if(match(ShortcutAction::SquadFight))controller_.RequestSquad(false);
                else if(match(ShortcutAction::SquadFollow))controller_.RequestSquad(true);
                else if(match(ShortcutAction::SquadRelease))controller_.RequestSquadRelease();
                else for(int i=0;i<4;++i)if(match(ShortcutAction(int(ShortcutAction::Spell1)+i)))controller_.RequestSpell(i);
            }
        }
        // Native HUD stays visible for portraits, but unused keyboard input must not issue RTS orders in FPS.
        return 0;
    }
    if(message==WM_CHAR || message==WM_SYSCHAR || message==WM_LBUTTONDOWN || message==WM_LBUTTONUP ||
        message==WM_RBUTTONDOWN || message==WM_RBUTTONUP || message==WM_MOUSEMOVE)return 0;
    return std::nullopt;
}
