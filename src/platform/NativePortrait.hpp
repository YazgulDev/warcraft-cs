#pragma once
#include <windows.h>
#include <gl/GL.h>
#include <cstdint>

// Redirect the native animated portrait to its own color/depth target, leaving world pixels intact.
class NativePortrait {
public:
    void BeginFrame(int width,int height,uint32_t actor);
    bool Viewport(int x,int y,int width,int height);
    bool Capturing() const {return capturing_;}
    void Scissor(int& x,int& y) const {if(capturing_) {x-=x_;y-=y_;}}
    void Submitted() {if(capturing_)drawn_=true;}
    void Finish();
    void Draw(int x,int y,int side);
    void Reset() {actor_=0;ready_=false;}
    void ContextDeleted(HGLRC context);
private:
    bool Prepare(int width,int height);
    int width_=0,height_=0,x_=0,y_=0,cropWidth_=0,cropHeight_=0,side_=0;
    uint32_t actor_=0;
    bool capturing_=false,drawn_=false,ready_=false;
    HGLRC context_=nullptr;
    GLuint framebuffer_=0,texture_=0,depth_=0;
    GLint previous_=0;
};
