#pragma once

// Ground-height presentation is independent of collision, mouse input and jump velocity.
struct CameraHeightSettings {
    bool enabled=true;
    float seconds=.08f;
};
