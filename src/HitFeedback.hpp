#pragma once
#include <windows.h>

// Confirmed hits refresh one neutral marker without blood or particle allocations.
class HitFeedback {
public:
    void Record();
    void Clear();
    void Draw(float width, float height) const;
private:
    DWORD tick_ = 0;
    bool active_ = false;
};
