#pragma once

// Predict queued semicolon toggles so a quick key+RMB sequence uses the mode the player just selected.
class SquadOrderMode {
public:
    void Configure(bool enabled) {enabled_=enabled;pending_=false;}
    void QueueToggle() {pending_=!pending_;}
    bool RoutingEnabled() const {return enabled_!=pending_;}
    bool Enabled() const {return enabled_;}
    void ClearInput() {pending_=false;}
    bool Apply(bool allowed) {
        if(!pending_)return false;
        pending_=false;if(!allowed)return false;
        enabled_=!enabled_;return true;
    }
private:
    bool enabled_=false,pending_=false;
};
