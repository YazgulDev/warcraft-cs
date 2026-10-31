#pragma once

// Granting and launching are separate states: chat Enter can never detonate the reward.
class NuclearInventory {
public:
    bool Grant(bool enabled,bool repeat){if(!enabled || (!available_ && used_ && !repeat))return false;available_=selected_=true;return true;}
    bool Launch(){if(!selected_ || !available_)return false;available_=selected_=false;used_=true;return true;}
    void Holster(){selected_=false;}
    bool Selected() const{return selected_;}
    bool Available() const{return available_;}
    void Reset(){*this={};}
private:
    bool available_=false,selected_=false,used_=false;
};
