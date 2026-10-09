#include "../src/presentation/FullscreenView.hpp"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
    // Native world rectangles fill the client in widescreen and classic 4:3 modes.
    for (auto dimensions : {1920,1024}) {
        int height=dimensions==1920 ? 1080:768;
        int x=0,y=height/4,w=dimensions,h=height*2/3;
        FullscreenView::Expand(dimensions,height,x,y,w,h);
        assert(x==0 && y==0 && w==dimensions && h==height);
    }
    // Portraits must retain their own small viewport even while FPS owns the world.
    int x=512,y=20,w=192,h=192;
    FullscreenView::Expand(1920,1080,x,y,w,h);
    assert(x==512 && y==20 && w==192 && h==192);
    FullscreenView::Expand(0,0,x,y,w,h);
    assert(x==512 && y==20 && w==192 && h==192);
    std::puts("Fullscreen world and native portrait viewport checks passed.");
}
