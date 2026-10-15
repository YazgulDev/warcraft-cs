#pragma once
#include "../combat/UnitAimTarget.hpp"
#include "WorldLabelView.hpp"

// Rendering uses a contact snapshot only; it never selects a unit or invokes Warcraft gameplay natives.
class TargetMarkerView {
public:
    static bool Draw(const UnitAimTarget& target,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane);
};
