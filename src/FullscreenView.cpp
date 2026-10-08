#include "FullscreenView.hpp"

void FullscreenView::Expand(int clientWidth, int clientHeight, int& x, int& y, int& width, int& height) {
    // Warcraft submits its world separately from small portrait/minimap viewports.
    // Fullscreen UI rectangles remain identical; smaller native viewports stay intact.
    if (clientWidth<=0 || clientHeight<=0 || width<=clientWidth*3/4 || height<=clientHeight/2) return;
    x=y=0; width=clientWidth; height=clientHeight;
}
