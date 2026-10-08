#pragma once
#include "ShooterController.hpp"
#include <functional>

namespace BuyMenuView {
using Text=std::function<void(float,float,const char*)>;
using Preview=std::function<void(int,float,float,float,float)>;
// Draw keyboard/mouse rows from the same layout used by input hit-testing.
void Draw(const ShooterController& controller,int width,int height,HWND window,const Text& text,const Text& titleText,const Preview& preview);
}
