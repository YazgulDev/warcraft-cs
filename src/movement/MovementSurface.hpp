#pragma once

// A local contact, not the height of a distant terrace. Only walkable contacts allow another jump.
struct MovementSurface {
    float height=0;
    float normal[3]={0,0,1};
    bool walkable=true;
    bool ramp=false;
};
