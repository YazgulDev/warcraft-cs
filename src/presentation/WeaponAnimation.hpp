#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>

namespace WeaponAnimation {
// Reload transitions belong to the controller; rendering must never insert an idle pose between shells.
inline bool HoldLastFrame(std::string_view name) { return name=="start_reload" || name=="insert" || name=="after_reload" || name=="pullpin" || name=="throw"; }
inline float Frame(unsigned frames,float fps,float seconds,bool loop) {
    float frame=std::max(0.f,seconds)*fps;
    return loop ? std::fmod(frame,float(frames)) : std::min(frame,float(frames-1));
}
}
