#pragma once

// Equipment is stored; only consumed powerups qualify for the existing rune ammunition reward.
struct ItemPickupResult {
    bool found=false;
    bool accepted=false;
    bool powerup=false;
};
