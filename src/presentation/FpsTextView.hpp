#pragma once
#include "../input/FpsTextEntry.hpp"
#include "HudFont.hpp"

// UTF-8 wrapping and rendering are independent of native message submission and combat state.
namespace FpsTextView {void Draw(const FpsTextEntry& entry,const std::string& lastMessage,bool showLast,int width,int height,HudFont& font);}
