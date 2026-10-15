#pragma once
#include <windows.h>
#include <gl/GL.h>
#include <cstdint>
#include "../presentation/MinimapLayout.hpp"

// Capture the native map without changing anchors, visibility rules or mission bounds.
class NativeMinimap {
public:
    using DrawElements=void (APIENTRY*)(GLenum,GLsizei,GLenum,const void*);
    void BeginFrame(uintptr_t ui,int width,int height,bool enabled);
    void Submit(GLenum mode,GLsizei count,GLenum type,const void* indices,DrawElements original);
    void FinishFrame();
    void Draw(const MinimapLayout& layout);
    void ContextDeleted(HGLRC context);
private:
    bool Prepare();
    bool MapBatch(GLsizei count,GLenum type,const void* indices) const;
    void Quad(GLuint texture,float left,float bottom,float right,float top,bool inset=true);
    uintptr_t ui_=0;
    int width_=0,height_=0,x_=0,y_=0,cropWidth_=0,cropHeight_=0,textureSide_=0;
    bool enabled_=false,capturing_=false,ready_=false;
    HGLRC context_=nullptr;
    GLuint background_=0,map_=0;
    GLenum captureBuffer_=GL_BACK;
};
