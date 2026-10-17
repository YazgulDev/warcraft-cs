#pragma once
#include "../inventory/InventoryControls.hpp"
#include "../platform/NativeItemActions.hpp"
#include "../config/GameplaySettings.hpp"
#include "../presentation/InventoryHudLayout.hpp"
#include <functional>

// Coordinate inventory gestures/native actions separately from movement, weapons and Windows messages.
class InventoryController {
public:
    using Aim=std::function<SpellAim(int,int,int,int)>;
    void Configure(uintptr_t base) {actions_.Configure(base);}
    void Input(InventoryControls::Event e) {controls_.Queue(e);}
    const InventoryControls& State() const {return controls_;}
    void Reset() {ClearInput();controls_.Reset(defaultNumbers_);actions_.Reset();}
    void ApplySettings(const GameplaySettings& settings);
    void ClearInput();
    bool Active() const {return actions_.Active();}
    void CancelCasting() {actions_.Cancel();}
    const char* Update(wc3::Handle actor,const GameplaySettings& settings,bool blocked,float yaw,bool self,
        const Aim& pick,const std::function<void()>& prepare);
private:
    InventoryControls controls_;
    NativeItemActions actions_;
    bool defaultNumbers_=true;
};
