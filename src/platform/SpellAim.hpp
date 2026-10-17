#pragma once
#include "../combat/UnitHitboxes.hpp"
#include "../combat/DestructableHitboxes.hpp"
#include <functional>

// Widget selection and the ground point are independent; point spells never snap to a widget's surface.
struct SpellAim {
    wc3::Handle target=0;
    float x=0,y=0;
    bool point=false;
    static SpellAim Pick(wc3::Handle actor,const float* origin,const float* direction,float distance,
        UnitHitboxes& units,DestructableHitboxes& destructables,
        const std::function<bool(wc3::Handle)>& unitFilter={}); // Actions filter candidates before they can occlude the ray.
};
