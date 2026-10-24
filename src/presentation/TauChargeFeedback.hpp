#pragma once
#include "../config/HalfLifeSettings.hpp"
#include <algorithm>

// Integrate a charge-driven speed ramp: changing the rate must never jump the spinner's animation phase.
namespace TauChargeFeedback {
inline float Fraction(float elapsed,const HalfLifeSettings& settings) {return std::clamp(elapsed/settings.tauChargeSeconds,0.f,1.f);}
inline float Rate(float elapsed,const HalfLifeSettings& settings) {
    return settings.tauSpinStartRate+(settings.tauSpinEndRate-settings.tauSpinStartRate)*Fraction(elapsed,settings);
}
inline float Pitch(float elapsed,const HalfLifeSettings& settings) {
    return settings.tauPitchStart+(settings.tauPitchEnd-settings.tauPitchStart)*Fraction(elapsed,settings);
}
inline float Phase(float elapsed,const HalfLifeSettings& settings) {
    float t=std::clamp(elapsed,0.f,settings.tauChargeSeconds);
    return settings.tauSpinStartRate*t+(settings.tauSpinEndRate-settings.tauSpinStartRate)*t*t/(2*settings.tauChargeSeconds)
        +std::max(0.f,elapsed-settings.tauChargeSeconds)*settings.tauSpinEndRate;
}
inline float SpinSeconds(float elapsed,float spinup,const HalfLifeSettings& settings) {
    return elapsed>spinup ? Phase(elapsed,settings)-Phase(spinup,settings) : 0;
}
}
