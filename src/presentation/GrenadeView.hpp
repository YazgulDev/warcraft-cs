#pragma once
#include "WorldLabelView.hpp"
#include "../combat/GrenadeVisual.hpp"
#include <vector>

namespace GrenadeView {
void Draw(const std::vector<GrenadeVisual>& items,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane);
void Flash(float opacity,int width,int height);
}
