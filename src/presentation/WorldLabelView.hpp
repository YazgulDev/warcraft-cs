#pragma once

// Per-frame camera data keeps label policy independent of Warcraft and OpenGL.
struct WorldLabelView {
    bool active=false;
    float eye[3]={};
    float yaw=0, pitch=0, verticalFov=85, aspect=1;
    float maximumDistance=1200;
};
