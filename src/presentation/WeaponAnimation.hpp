#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>

namespace WeaponAnimation {
// Hold primed grenades and action end poses until the controller advances the animation.
inline bool HoldLastFrame(std::string_view name) {
    return name=="start_reload" || name=="insert" || name=="after_reload" ||
        name=="pullpin" || name=="throw" || name=="pinpull" || name=="throw2" || name=="spinup" || name=="altfireon";
}
inline float Frame(unsigned frames,float fps,float seconds,bool loop) {
    float frame=std::max(0.f,seconds)*fps;
    return loop ? std::fmod(frame,float(frames)) : std::min(frame,float(frames-1));
}
inline float HeldActionFrame(std::string_view name,float frame) {
    // The stock Half-Life pinpull leaves the screen after frame 22. Hold its last visible primed pose.
    return name=="pinpull" ? std::min(frame,22.f) : frame;
}
}
