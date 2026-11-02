#pragma once

// One validated snapshot supplies native frame pacing and the FPS world camera, reloaded on F8.
struct RenderSettings {
    static constexpr float MinimumFov=60,MaximumFov=120,DefaultFov=85;
    int maxFPS=144;
    // Default world visibility is 5000 Warcraft units; explicit saved distances remain editable.
    float drawDistance=5000;
    // Horizontal world-camera degrees; scopes retain their authored zoom independently of this preference.
    float fieldOfView=DefaultFov;
    float Fov(int scopeLevel=0) const {return scopeLevel==2 ? 10.f : scopeLevel ? 40.f : fieldOfView;}
};
