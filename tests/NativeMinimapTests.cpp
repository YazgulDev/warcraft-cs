#include "../src/platform/NativeMinimap.hpp"
#include <cassert>
#include <vector>
namespace wc3 {void Log(const char*,...) {}}
static int draws=0;
static GLenum targetBuffer=GL_BACK;
static void APIENTRY DrawMap(GLenum mode,GLsizei count,GLenum type,const void* indices) {
    // Synthetic native batch obeys the same scissor contract as Warcraft's terrain/marker batches.
    ++draws;glColor3f(1,0,0);glDrawElements(mode,count,type,indices);
}
static std::vector<unsigned char> Pixels() {
    std::vector<unsigned char> pixels(320*240*3);glReadBuffer(targetBuffer);
    glReadPixels(0,0,320,240,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());return pixels;
}
int main() {
    // Hidden double-buffered Win32 context verifies real GL copy/restore without game data or user input.
    WNDCLASSA cls={};cls.lpfnWndProc=DefWindowProcA;cls.hInstance=GetModuleHandleA(nullptr);cls.lpszClassName="WcsMinimapOracle";
    assert(RegisterClassA(&cls));HWND window=CreateWindowA(cls.lpszClassName,"",WS_POPUP,0,0,320,240,nullptr,nullptr,cls.hInstance,nullptr);
    assert(window);HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=24;format.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));HGLRC context=wglCreateContext(dc);assert(wglMakeCurrent(dc,context));
    // Exercise restored front-buffer frames as well as ordinary back-buffer frames with real pixels.
    for(GLenum buffer : {GL_BACK,GL_FRONT}) {
    targetBuffer=buffer;glDrawBuffer(buffer);draws=0;
    glViewport(0,0,320,240);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,320,0,240,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    glDisable(GL_DITHER);glClearColor(.1f,.2f,.3f,1);glClear(GL_COLOR_BUFFER_BIT);
    // Distinct pixel stripes detect interpolation, pixel shifts and restoration outside the native map.
    for (int x=0;x<320;++x) {glColor3f(x%2 ? .8f : .2f,float(x)/320,.4f);glBegin(GL_QUADS);
        glVertex2i(x,0);glVertex2i(x+1,0);glVertex2i(x+1,240);glVertex2i(x,240);glEnd();}
    // Combined minimap/health rendering retains the world depth, even if a native batch requests writes.
    glClearDepth(.37);glClear(GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_TRUE);
    std::vector<float> depthBefore(320*240),depthAfter(320*240);
    glReadPixels(0,0,320,240,GL_DEPTH_COMPONENT,GL_FLOAT,depthBefore.data());
    auto before=Pixels();uintptr_t ui[512]={};float frame[256]={};
    ui[0x3C0/sizeof(uintptr_t)]=reinterpret_cast<uintptr_t>(frame);
    float* bounds=frame+(0xB4+0x44)/sizeof(float);bounds[0]=.02f;bounds[1]=.02f;bounds[2]=.32f;bounds[3]=.32f;
    NativeMinimap map;map.BeginFrame(reinterpret_cast<uintptr_t>(ui),320,240,true);
    GLfloat console[]={0,0,319,0,319,239,0,239};GLushort indices[]={0,1,2,0,2,3};
    glEnableClientState(GL_VERTEX_ARRAY);glVertexPointer(2,GL_FLOAT,0,console);
    map.Submit(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,indices,DrawMap);assert(draws==0);
    // A rectangular map retains black letterbox strips, while native edge overscan remains accepted.
    GLfloat vertices[]={5,20,127,20,127,110,5,110};glVertexPointer(2,GL_FLOAT,0,vertices);
    map.Submit(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,indices,DrawMap);assert(Pixels()!=before);
    assert(draws==1);
    // Completion can select a different HUD target; restoration still belongs to the captured UI buffer.
    GLenum hudBuffer=buffer==GL_FRONT ? GL_BACK : GL_FRONT;glDrawBuffer(hudBuffer);
    map.FinishFrame();assert(Pixels()==before);
    GLint restored=0;glGetIntegerv(GL_DRAW_BUFFER,&restored);assert(restored==hudBuffer);glDrawBuffer(buffer);
    glReadPixels(0,0,320,240,GL_DEPTH_COMPONENT,GL_FLOAT,depthAfter.data());assert(depthBefore==depthAfter);
    GLboolean depthMask=GL_FALSE;glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);assert(depthMask==GL_TRUE);
    glDisable(GL_DEPTH_TEST); // Background must be restored pixel for pixel.
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,320,240,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    auto layout=MinimapLayout::Calculate(320,240,22);map.Draw(layout);auto displayed=Pixels();
    int center=((240-layout.y-layout.side/2)*320+layout.x+layout.side/2)*3;
    assert(displayed[center]==255 && displayed[center+1]==0 && displayed[center+2]==0);
    int letterbox=((240-layout.y-2)*320+layout.x+layout.side/2)*3;
    assert(displayed[letterbox]==0 && displayed[letterbox+1]==0 && displayed[letterbox+2]==0);
    map.BeginFrame(reinterpret_cast<uintptr_t>(ui),320,240,false);
    auto hidden=Pixels();map.Submit(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,indices,DrawMap);map.FinishFrame();map.Draw(layout);assert(Pixels()==hidden);
    assert(glGetError()==GL_NO_ERROR);map.ContextDeleted(context);
    }
    wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    ReleaseDC(window,dc);DestroyWindow(window);UnregisterClassA(cls.lpszClassName,cls.hInstance);
}
