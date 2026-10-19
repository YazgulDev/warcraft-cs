#pragma once
#include "../application/ShooterController.hpp"
#include <functional>

// CS-style category strip and owned-weapon stack; aim remains live and LMB confirms the highlight.
namespace WeaponSelectionView {
using Text=std::function<void(float,float,const char*)>;
using Preview=std::function<void(int,float,float,float,float)>;
void Draw(const ShooterController& controller,int width,int height,const Text& text,const Preview& preview);
}
