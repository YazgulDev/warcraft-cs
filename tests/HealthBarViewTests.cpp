#include "../src/presentation/HealthBarRenderer.hpp"
#include "../src/presentation/TargetMarkerView.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <cassert>
#include <cstdio>
#include <limits>

static int Colored(bool ally) {
    unsigned char pixels[320*240*3];glReadPixels(0,0,320,240,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    int count=0;for(int i=0;i<320*240*3;i+=3)
        if(ally ? (pixels[i+1]>200 && pixels[i]<100) : (pixels[i]>200 && pixels[i+1]<100)) ++count;
    return count;
}
int main() {
    WorldLabelView view;view.active=true;view.eye[2]=96;view.aspect=320.f/240;view.maximumDistance=2000;
    UnitHealthBar bar={{400,0,180},50,100,true};HealthBarPlacement at;
    assert(HealthBarView::Project(bar,view,320,240,8,15000,at));assert(at.x==160 && at.y<120 && at.fraction==.5f);
    bar.position[1]=80;assert(HealthBarView::Project(bar,view,320,240,8,15000,at));assert(at.x<160);
    float wide=at.x;view.verticalFov=40;assert(HealthBarView::Project(bar,view,320,240,8,15000,at));assert(at.x<wide);
    view.verticalFov=85;bar.position[0]=-400;assert(!HealthBarView::Project(bar,view,320,240,8,15000,at));
    bar.position[0]=400;view.maximumDistance=100;assert(!HealthBarView::Project(bar,view,320,240,8,15000,at));
    view.maximumDistance=2000;bar.maximum=0;assert(!HealthBarView::Project(bar,view,320,240,8,15000,at));
    bar.maximum=100;bar.health=std::numeric_limits<float>::quiet_NaN();assert(!HealthBarView::Project(bar,view,320,240,8,15000,at));
    bar.health=150;assert(HealthBarView::Project(bar,view,320,240,8,15000,at));assert(at.fraction==1);
    // Hidden WGL pixels prove filled colors, health fractions, depth occlusion and host-state restoration.
    HWND window=CreateWindowA("STATIC","Health bar regression",WS_POPUP,0,0,320,240,nullptr,nullptr,nullptr,nullptr);
    assert(window);HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR pf={};pf.nSize=sizeof(pf);pf.nVersion=1;
    pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=24;pf.cDepthBits=24;
    int pixel=ChoosePixelFormat(dc,&pf);assert(pixel && SetPixelFormat(dc,pixel,&pf));
    HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));glViewport(0,0,320,240);
    bar.position[1]=0;bar.health=100;
    glClearColor(0,0,0,1);glClearDepth(1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glEnable(GL_POLYGON_STIPPLE);glDepthFunc(GL_GREATER);
    assert(HealthBarRenderer::Draw({bar},view,320,240,8,15000)==1);int full=Colored(true);assert(full>200);
    GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && !mask[1] && !mask[2] && !mask[3]);
    GLint depthFunc;glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);assert(depthFunc==GL_GREATER && glIsEnabled(GL_POLYGON_STIPPLE));
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClear(GL_COLOR_BUFFER_BIT);bar.health=50;
    HealthBarRenderer::Draw({bar},view,320,240,8,15000);int half=Colored(true);assert(half>full*.4f && half<full*.6f);
    glClear(GL_COLOR_BUFFER_BIT);bar.ally=false;HealthBarRenderer::Draw({bar},view,320,240,8,15000);assert(Colored(false)>100);
    glClearDepth(.5);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    HealthBarRenderer::Draw({bar},view,320,240,8,15000);assert(Colored(false)==0);
    // Real WGL ring pixels prove enemy/yellow colors, terrain depth occlusion and caller-state restoration.
    glClearDepth(1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,320,240,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    UnitAimTarget target;target.handle=1;target.enemy=true;target.position[0]=-500; // Hide brackets to measure only the ground circle.
    for(size_t i=0;i<target.circle.size();++i) {
        float angle=float(i)*6.28318530718f/target.circle.size();
        target.circle[i]={400+64*std::cos(angle),64*std::sin(angle),3};
    }
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    assert(TargetMarkerView::Draw(target,view,320,240,8,15000));assert(Colored(false)>30);
    glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0]);
    glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);assert(depthFunc==GL_GREATER);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClear(GL_COLOR_BUFFER_BIT);
    target.enemy=false;TargetMarkerView::Draw(target,view,320,240,8,15000);
    unsigned char pixels[320*240*3];glReadPixels(0,0,320,240,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    int yellow=0;for(int i=0;i<320*240*3;i+=3)if(pixels[i]>200 && pixels[i+1]>200 && pixels[i+2]<100)++yellow;
    assert(yellow>30 && Colored(false)==0);
    glClearDepth(.5);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    target.enemy=true;TargetMarkerView::Draw(target,view,320,240,8,15000);assert(Colored(false)==0);
    assert(glGetError()==GL_NO_ERROR);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
    std::puts("Health bar projection, team pixels, fractions, occlusion and GL state passed.");
}
