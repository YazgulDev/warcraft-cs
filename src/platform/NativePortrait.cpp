#include "NativePortrait.hpp"
#include "WarcraftApi.hpp"
#ifdef WCS_D3D9
#include "D3DNativeUi.hpp"
#include "../presentation/D3DGraphics.hpp"
#endif
#include <algorithm>

namespace {
constexpr GLenum Framebuffer=0x8D40,Binding=0x8CA6,Renderbuffer=0x8D41,RenderBinding=0x8CA7;
constexpr GLenum Color=0x8CE0,Depth=0x8D00,Complete=0x8CD5;
using Gen=void (APIENTRY*)(GLsizei,GLuint*);
using Bind=void (APIENTRY*)(GLenum,GLuint);
using Image=void (APIENTRY*)(GLenum,GLenum,GLenum,GLuint,GLint);
using Storage=void (APIENTRY*)(GLenum,GLenum,GLsizei,GLsizei);
using Attach=void (APIENTRY*)(GLenum,GLenum,GLenum,GLuint);
using Status=GLenum (APIENTRY*)(GLenum);
template<class T>T Proc(const char* name) {return reinterpret_cast<T>(wglGetProcAddress(name));}
}
void NativePortrait::ContextDeleted(HGLRC context) {
    if(context_!=context)return;
    // IDs and extension entry points belong to the active WGL context.
    context_=nullptr;framebuffer_=texture_=depth_=0;side_=0;capturing_=ready_=false;
}
void NativePortrait::BeginFrame(int width,int height,uint32_t actor) {
    Finish();width_=width;height_=height;
    if(actor_!=actor)ready_=false;actor_=actor;
}
bool NativePortrait::Prepare(int width,int height) {
    auto gen=Proc<Gen>("glGenFramebuffersEXT"),genDepth=Proc<Gen>("glGenRenderbuffersEXT");
    auto bind=Proc<Bind>("glBindFramebufferEXT"),bindDepth=Proc<Bind>("glBindRenderbufferEXT");
    auto image=Proc<Image>("glFramebufferTexture2DEXT");auto storage=Proc<Storage>("glRenderbufferStorageEXT");
    auto attach=Proc<Attach>("glFramebufferRenderbufferEXT");auto status=Proc<Status>("glCheckFramebufferStatusEXT");
    if(!gen || !genDepth || !bind || !bindDepth || !image || !storage || !attach || !status)return false;
    int side=1;while(side<std::max(width,height))side*=2;
    GLint maximum=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum);if(side>maximum)return false;
    glGetIntegerv(Binding,&previous_);GLint render=0;glGetIntegerv(RenderBinding,&render);
    glPushAttrib(GL_TEXTURE_BIT);
    if(!framebuffer_) {gen(1,&framebuffer_);genDepth(1,&depth_);glGenTextures(1,&texture_);}
    bind(Framebuffer,framebuffer_);
    if(side_!=side) {
        side_=side;glBindTexture(GL_TEXTURE_2D,texture_);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,side,side,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        image(Framebuffer,Color,GL_TEXTURE_2D,texture_,0);bindDepth(Renderbuffer,depth_);
        storage(Renderbuffer,0x81A6,side,side);attach(Framebuffer,Depth,Renderbuffer,depth_);
        wc3::Log("Portrait native crop=%dx%d texture=%d",width,height,side);
    }
    bool complete=status(Framebuffer)==Complete;
    bindDepth(Renderbuffer,GLuint(render));glPopAttrib();
    if(!complete) {bind(Framebuffer,GLuint(previous_));return false;}
    // Clear only the private target: native portrait clears must never erase world depth or color.
    glPushAttrib(GL_SCISSOR_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(GL_TRUE);
    glClearColor(0,0,0,1);glClearDepth(1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glPopAttrib();
    return true;
}
bool NativePortrait::Viewport(int x,int y,int width,int height) {
    Finish();if(!actor_ || width_<=0 || height_<=0)return false;
    auto current=wglGetCurrentContext();if(!current)return false;
    if(context_!=current) {ContextDeleted(context_);context_=current;}
    // Warcraft's portrait is the small bottom console 3D viewport, separate from full-screen UI batches.
    if(width<16 || height<16 || width>width_*.25f || height>height_*.35f ||
        x<width_*.15f || x>width_*.55f || y<0 || y>height_*.3f)return false;
    if(!Prepare(width,height))return false;
    x_=x;y_=y;cropWidth_=width;cropHeight_=height;capturing_=true;drawn_=false;return true;
}
void NativePortrait::Finish() {
    if(!capturing_)return;
    if(context_==wglGetCurrentContext()) {
        if(auto bind=Proc<Bind>("glBindFramebufferEXT"))bind(Framebuffer,GLuint(previous_));
        if(drawn_)ready_=true;
    }
    capturing_=false;
}
void NativePortrait::Draw(int x,int y,int side) {
#ifdef WCS_D3D9
    // Direct3D captures the same animated native portrait into a device-owned texture.
    if(D3DGraphics::Active()) {D3DNativeUi::DrawPortrait(x,y,side);return;}
#endif
    if(!ready_ || context_!=wglGetCurrentContext() || !glIsTexture(texture_))return;
    glPushAttrib(GL_TEXTURE_BIT|GL_CURRENT_BIT);glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture_);glColor4f(1,1,1,1);
    float u=(cropWidth_-.5f)/side_,v=(cropHeight_-.5f)/side_,edge=.5f/side_;
    glBegin(GL_QUADS);glTexCoord2f(edge,v);glVertex2i(x,y);glTexCoord2f(u,v);glVertex2i(x+side,y);
    glTexCoord2f(u,edge);glVertex2i(x+side,y+side);glTexCoord2f(edge,edge);glVertex2i(x,y+side);glEnd();glPopAttrib();
}
