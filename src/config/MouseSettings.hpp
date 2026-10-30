#pragma once

// Unit multipliers preserve the original unscoped and two scoped look speeds by default.
struct MouseSettings {
    float sensitivity=1,scopeSensitivity=1;
    float Gain(int scope) const {
        float base=scope==2 ? .009f : scope ? .035f : .14f;
        return base*sensitivity*(scope ? scopeSensitivity : 1.f);
    }
};
