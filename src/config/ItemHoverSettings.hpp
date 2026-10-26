#pragma once

// Names are a reticle query, independent of pickup range or native inventory capacity.
struct ItemHoverSettings {
    bool enabled=true;
    float distance=1200,radius=32;
};
