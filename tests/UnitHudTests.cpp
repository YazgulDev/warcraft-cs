#include "../src/presentation/UnitHudLayout.hpp"
#include "../src/presentation/InventoryHudLayout.hpp"
#include "../src/platform/NativePortrait.hpp"
#include <cassert>
#include <vector>
namespace wc3 {void Log(const char*,...) {}}
void TestNativeUnitHud();

int main() {
    TestNativeUnitHud();
    for(auto size : {std::pair<int,int>{640,480},{1280,720},{1920,1080},{2560,1080}})
    for(float scale : {60.f,100.f,160.f}) {
        auto l=UnitHudLayout::Calculate(size.first,size.second,scale);
        assert(l.x>=0 && l.y>0 && l.x+l.portrait+l.gap+l.bars<size.first && l.y+l.portrait<size.second);
        for(float icon : {4.f,6.f,10.f}) {
            auto items=InventoryHudLayout::Calculate(size.first,size.second,icon,true);
            auto spells=SpellHudLayout::Calculate(size.first,size.second,icon);
            assert(items.x>=0 && items.y>0 && items.y+items.side<spells.y-7);
            assert(items.x+6*items.side+5*items.gap<size.first);
        }
    }
    // A hidden real WGL context proves portrait color/depth are isolated from the world.
    WNDCLASSA cls={};cls.lpfnWndProc=DefWindowProcA;cls.hInstance=GetModuleHandleA(nullptr);cls.lpszClassName="WcsPortraitOracle";
    assert(RegisterClassA(&cls));HWND window=CreateWindowA(cls.lpszClassName,"",WS_POPUP,0,0,640,480,nullptr,nullptr,cls.hInstance,nullptr);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=24;format.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));HGLRC context=wglCreateContext(dc);assert(wglMakeCurrent(dc,context));
    glViewport(0,0,640,480);glClearColor(.1f,.2f,.3f,1);glClearDepth(.37);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    std::vector<unsigned char> before(640*480*3),after(before.size());std::vector<float> depth(before.size()/3),depthAfter(depth.size());
    glReadPixels(0,0,640,480,GL_RGB,GL_UNSIGNED_BYTE,before.data());glReadPixels(0,0,640,480,GL_DEPTH_COMPONENT,GL_FLOAT,depth.data());
    NativePortrait portrait;portrait.BeginFrame(640,480,1);assert(!portrait.Viewport(0,0,640,480));
    assert(portrait.Viewport(160,20,80,80));glViewport(0,0,80,80);
    glClearColor(1,0,0,1);glClearDepth(1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);portrait.Submitted();portrait.Finish();
    glReadPixels(0,0,640,480,GL_RGB,GL_UNSIGNED_BYTE,after.data());glReadPixels(0,0,640,480,GL_DEPTH_COMPONENT,GL_FLOAT,depthAfter.data());
    assert(before==after && depth==depthAfter);
    glViewport(0,0,640,480);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,640,480,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    portrait.Draw(20,360,80);glReadPixels(0,0,640,480,GL_RGB,GL_UNSIGNED_BYTE,after.data());
    int sample=((480-400)*640+60)*3;assert(after[sample]==255 && after[sample+1]==0);
    // Another native animation frame replaces the pixels; F9/actor changes cannot reuse a stale face.
    portrait.BeginFrame(640,480,1);assert(portrait.Viewport(160,20,80,80));
    glClearColor(0,1,0,1);glClear(GL_COLOR_BUFFER_BIT);portrait.Submitted();portrait.Finish();
    portrait.Draw(20,360,80);glReadPixels(0,0,640,480,GL_RGB,GL_UNSIGNED_BYTE,after.data());assert(after[sample]==0 && after[sample+1]==255);
    portrait.BeginFrame(640,480,0);assert(!portrait.Viewport(160,20,80,80));
    before=after;portrait.Draw(20,360,80);glReadPixels(0,0,640,480,GL_RGB,GL_UNSIGNED_BYTE,after.data());assert(before==after);
    assert(glGetError()==GL_NO_ERROR);portrait.ContextDeleted(context);
    wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
}
