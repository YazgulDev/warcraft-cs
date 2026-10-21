#include "../src/presentation/EnergyBeamView.hpp"
#include <cassert>
#include <vector>
#include <cstdio>

int main() {
    HWND window=CreateWindowA("STATIC","Energy beam",WS_POPUP,0,0,256,256,nullptr,nullptr,nullptr,nullptr);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;
    format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;format.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));
    for(int cycle=0;cycle<2;++cycle) {
        HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));glViewport(0,0,256,256);
        WorldLabelView view;view.active=true;view.eye[2]=96;
        EnergyBeam beam;beam.start={50,-30,96};beam.end={100,30,96};beam.tick=GetTickCount();beam.seconds=1;beam.width=3;
        auto clear=[](float depth) {glColorMask(1,1,1,1);glDepthMask(1);glClearDepth(depth);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);};
        auto ink=[] {std::vector<unsigned char> pixels(256*256*4);glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());int result=0;
            for(size_t i=0;i<pixels.size();i+=4)if(pixels[i]>20 || pixels[i+1]>20 || pixels[i+2]>20)++result;return result;};
        for(bool gluon:{false,true}) {
            beam.gluon=gluon;clear(1);EnergyBeamView::Draw(beam,view,8,5000);assert(ink()>50);
            clear(0);EnergyBeamView::Draw(beam,view,8,5000);assert(ink()==0); // Native geometry occludes both beam colors.
        }
        clear(1);glColorMask(0,0,0,0);glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glEnable(GL_COLOR_LOGIC_OP);glLogicOp(GL_NOOP);
        EnergyBeamView::Draw(beam,view,8,5000);assert(ink()>50);
        GLboolean mask[4]={};glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && glIsEnabled(GL_COLOR_LOGIC_OP));
        GLint polygon[2]={};glGetIntegerv(GL_POLYGON_MODE,polygon);assert(polygon[0]==GL_LINE);
        glDisable(GL_COLOR_LOGIC_OP);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);clear(1);beam.tick=GetTickCount()-2000;
        EnergyBeamView::Draw(beam,view,8,5000);assert(ink()==0 && glGetError()==GL_NO_ERROR);
        wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    ReleaseDC(window,dc);DestroyWindow(window);
    std::puts("PASS real WGL Tau/Gluon beam pixels, occlusion, expiry, inherited state restoration and context replacement.");
}
