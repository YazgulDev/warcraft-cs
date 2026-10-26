#pragma once
#include "BuyMenuState.hpp"
#include <windows.h>
#include <functional>
class ShooterController;

namespace BuyMenuView {
using Text=std::function<void(float,float,const char*)>;
using Preview=std::function<void(int,float,float,float,float)>;
// Draw keyboard/mouse rows from the same layout used by input hit-testing.
void Draw(const ShooterController& controller,int width,int height,HWND window,const Text& text,const Text& titleText,const Preview& preview);
void Draw(const BuyMenuState& state,int width,int height,POINT cursor,const Text& text,const Text& titleText,const Preview& preview);
}
