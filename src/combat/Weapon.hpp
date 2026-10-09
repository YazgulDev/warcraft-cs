#pragma once

// Weapon definitions are combat data, independent of input dispatch and shooter lifecycle.
struct Weapon {
    const char* name;
    const char* sound;
    int magazine;
    int reserve;
    float damage;
    float interval;
    float reload;
    float range;
    bool automatic;
};
