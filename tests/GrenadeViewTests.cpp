#include "../src/presentation/GrenadeView.hpp"
#include "../src/combat/WeaponSlots.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <cassert>
#include <vector>

namespace {
void Clear(float depth=1){glColorMask(1,1,1,1);glDepthMask(1);glClearColor(0,0,0,1);glClearDepth(depth);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}
int Ink(){std::vector<unsigned char> p(256*256*4);glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,p.data());int n=0;for(size_t i=0;i<p.size();i+=4)if(p[i]>20)++n;return n;}
}
int main() {
    HWND window=CreateWindowA("STATIC","Grenade rendering",WS_POPUP,0,0,256,256,nullptr,nullptr,nullptr,nullptr);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;format.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));glViewport(0,0,256,256);
    WorldLabelView view;view.active=true;view.eye[2]=96;
    // The smoke pass no longer paints substitute flight discs; the dedicated world-model pass owns them.
    Clear();GrenadeView::Draw({{{100,0,96},WeaponSlots::HE,5,1,0,false}},view,256,256,8,5000);assert(Ink()==0);
    // World smoke is visible and depth-occluded; entering it adds near-plane density instead of a clear hole.
    Clear();GrenadeView::Draw({{{800,0,0},WeaponSlots::Smoke,300,.85f,2,true}},view,256,256,8,5000);assert(Ink()>500);
    Clear(0);GrenadeView::Draw({{{800,0,0},WeaponSlots::Smoke,300,.85f,2,true}},view,256,256,8,5000);assert(Ink()==0);
    Clear();GrenadeView::Draw({{{100,0,0},WeaponSlots::Smoke,300,.85f,2,true}},view,256,256,8,5000);assert(Ink()==256*256);
    Clear();glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glColorMask(0,0,0,0);glEnable(GL_COLOR_LOGIC_OP);glLogicOp(GL_NOOP);
    GrenadeView::Flash(1,256,256);assert(Ink()==256*256);GLboolean mask[4]={};glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && glIsEnabled(GL_COLOR_LOGIC_OP));
    assert(glGetError()==GL_NO_ERROR);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
}
