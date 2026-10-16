#pragma once
#include "UnitHudSnapshot.hpp"
#include "UnitHudLayout.hpp"
#include <functional>
class NativePortrait;

class UnitHudView {
public:
    using Text=std::function<void(float,float,const char*)>;
    static void Draw(const UnitHudSnapshot& unit,const UnitHudLayout& layout,NativePortrait& portrait,const Text& text);
};
