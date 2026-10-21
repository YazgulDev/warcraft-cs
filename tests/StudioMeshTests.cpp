#include "../src/presentation/GunMesh.hpp"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstring>
namespace wc3 {void Log(const char*,...) {}}
template<class T> void Write(std::ofstream& file,const T& value){file.write(reinterpret_cast<const char*>(&value),sizeof(value));}
void Fixture(const char* path,int normalBone=1) {
    std::ofstream f(path,std::ios::binary);for(unsigned n:{0x33474357u,1u,1u,1u,2u})Write(f,n);Write(f,-1);Write(f,0);
    for(unsigned n:{3u,4u,1u,0xff0000ffu,0xff00ff00u,0xffff0000u,0xffffffffu,0u,6u})Write(f,n);
    struct Vertex {float x,y,z,u,v;unsigned bone;};
    for(auto v:{Vertex{20,-8,-8,0,0,0},Vertex{20,8,-8,0,0,0},Vertex{20,8,8,0,0,0},Vertex{20,-8,-8,0,0,0},Vertex{20,8,8,0,0,0},Vertex{20,-8,8,0,0,0}})Write(f,v);
    for(int i=0;i<6;++i){Write(f,0.f);Write(f,1.f);Write(f,0.f);Write(f,unsigned(normalBone));}
    char label[32]="spin";f.write(label,32);Write(f,1.f);Write(f,3u);Write(f,1u);
    for(int frame=0;frame<3;++frame){for(float n:{1.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f,1.f,0.f})Write(f,n);
        float sign=frame==1 ? -1.f : 1.f;for(float n:{sign,0.f,0.f,0.f,0.f,sign,0.f,0.f,0.f,0.f,1.f,0.f})Write(f,n);}
    Write(f,0x314D4357u);Write(f,0u);
}
void Timeline(const char* path) {
    std::ofstream f(path,std::ios::binary);for(unsigned n:{0x33474357u,1u,0u,3u,1u})Write(f,n);Write(f,-1);
    for(unsigned n:{0u,1u,1u,0xffffffffu})Write(f,n);
    for(const char* name:{"idle","spinup","attack"}) {
        char label[32]={};strcpy_s(label,name);f.write(label,32);Write(f,1.f);Write(f,2u);Write(f,unsigned(!strcmp(name,"attack") ? 0 : 1));
        for(int frame=0;frame<2;++frame){float x=strcmp(name,"idle") && frame ? 10.f : 0.f;
            for(float n:{1.f,0.f,0.f,x,0.f,1.f,0.f,0.f,0.f,0.f,1.f,0.f})Write(f,n);}
    }
    Write(f,0x314D4357u);Write(f,1u);Write(f,0u);Write(f,20.f);Write(f,0.f);Write(f,0.f);
}
int main() {
    HWND w=CreateWindowA("STATIC","Studio materials",WS_POPUP,0,0,128,128,0,0,0,0);HDC dc=GetDC(w);
    PIXELFORMATDESCRIPTOR p={};p.nSize=sizeof(p);p.nVersion=1;p.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;p.iPixelType=PFD_TYPE_RGBA;p.cColorBits=32;p.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&p),&p));HGLRC context=wglCreateContext(dc);assert(wglMakeCurrent(dc,context));
    glViewport(0,0,128,128);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(-32,32,-32,32,1,200);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    Fixture("studio-mesh.wcg");GunMesh mesh;assert(mesh.Load("studio-mesh.wcg"));
    auto render=[&](float seconds){glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);mesh.Draw("spin",seconds);
        std::vector<unsigned char> pixels(128*128*4);glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());return pixels;};
    auto before=render(0),rotated=render(1);size_t blueBefore=0,blueAfter=0;
    for(size_t i=2;i<before.size();i+=4){blueBefore+=before[i];blueAfter+=rotated[i];}
    assert(blueBefore>10000 && blueAfter<blueBefore/10);assert(render(2)==before); // Animated chrome and duplicate-terminal loop closure.
    Timeline("studio-timeline.wcg");assert(mesh.Load("studio-timeline.wcg"));std::array<float,3> anchor;
    assert(mesh.MuzzlePoint("spinup",5,1,anchor) && anchor[2]==-30); // No charge-to-idle blink.
    assert(mesh.MuzzlePoint("attack",1.5f,1,anchor) && anchor[2]==-30); // Complete final frame.
    assert(mesh.MuzzlePoint("attack",2.1f,1,anchor) && anchor[2]==-20);
    mesh.Reset(true);Fixture("studio-bad.wcg",99);assert(!mesh.Load("studio-bad.wcg"));mesh.Reset(true);
    assert(glGetError()==GL_NO_ERROR);wglMakeCurrent(0,0);wglDeleteContext(context);ReleaseDC(w,dc);DestroyWindow(w);
    std::remove("studio-mesh.wcg");std::remove("studio-bad.wcg");
    std::remove("studio-timeline.wcg");
    std::puts("PASS WCG3 actual chrome pixels, normal bones, seamless studio loop and malformed cache rejection.");
}
