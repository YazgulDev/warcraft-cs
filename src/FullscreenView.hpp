#pragma once

// Expand the main world rectangle without moving native UI/portrait frame anchors.
class FullscreenView {
public:
    static void Expand(int clientWidth, int clientHeight, int& x, int& y, int& width, int& height);
};
