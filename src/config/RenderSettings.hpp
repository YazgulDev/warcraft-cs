#pragma once

// One validated snapshot supplies native frame pacing and the FPS world camera, reloaded on F8.
struct RenderSettings {
    int maxFPS=144;
    // Default world visibility is 5000 Warcraft units; explicit saved distances remain editable.
    float drawDistance=5000;
};
