#pragma once
#include "../config/SettingsMenu.hpp"
#include <functional>

// Presentation consumes only the copied draft, never INI files or native unit state.
namespace SettingsMenuView {
using Text=std::function<void(float,float,const char*)>;
void Draw(const SettingsMenu& menu,int width,int height,int cursorX,int cursorY,const Text& text);
}
