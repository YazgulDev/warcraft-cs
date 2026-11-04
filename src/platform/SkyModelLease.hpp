#pragma once
#include <optional>
#include <string>

// Own only our last submitted model. Native map-script replacements become the new restoration baseline.
class SkyModelLease {
public:
    void Reset(){owned_=false;model_.clear();map_.clear();}
    std::optional<std::string> Update(const std::string& current,bool sprite,bool enabled,bool hidden,
        const char* selected,const char* fallback) {
        if(owned_ && current!=model_)Reset();
        const std::string original=owned_ ? map_ : current;
        std::string desired=original;
        if(hidden)desired.clear();
        else if(enabled && selected && *selected)desired=selected;
        else if(enabled && original.empty() && (owned_ || !sprite))desired=fallback;
        if(desired==current){if(owned_ && desired==original)Reset();return {};}
        if(!owned_){map_=current;owned_=true;}
        model_=desired;
        if(desired==map_)Reset(); // Restoration is complete; do not retain ownership over the map's model.
        return desired;
    }
private:
    bool owned_=false;
    std::string model_,map_;
};
