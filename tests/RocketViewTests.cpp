#include "../src/presentation/RocketView.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>
namespace wc3 {void Log(const char*,...) {}}
namespace {
template<class T> void Write(std::ofstream& file,const T& value){file.write(reinterpret_cast<const char*>(&value),sizeof(value));}
void Fixture(const std::filesystem::path& path,unsigned pixel) {
    std::ofstream file(path,std::ios::binary);
    for(unsigned value:{0x32474357u,1u,1u,1u,1u,1u,1u,pixel,0u,36u})Write(file,value);
    struct Vertex {float x,y,z,u,v;unsigned bone;};
    const int triangles[]={0,1,3,0,3,2,4,6,7,4,7,5,0,4,5,0,5,1,2,3,7,2,7,6,0,2,6,0,6,4,1,5,7,1,7,3};
    for(int corner:triangles){Vertex v={corner&1 ? 4.f : -4.f,corner&2 ? 7.f : -7.f,corner&4 ? 4.f : -4.f,0,0,0};Write(file,v);}
    char name[32]="idle";file.write(name,32);Write(file,30.f);Write(file,1u);
    // A nonzero authored model origin must not offset the flight relative to its physical center.
    for(float value:{1.f,0.f,0.f,30.f,0.f,1.f,0.f,25.f,0.f,0.f,1.f,0.f})Write(file,value);
}
void Clear(float depth=1){glColorMask(1,1,1,1);glDepthMask(1);glClearColor(0,0,0,1);glClearDepth(depth);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}
std::vector<unsigned char> Pixels(){std::vector<unsigned char> p(256*256*4);glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;}
int Ink(const std::vector<unsigned char>& p,int left=0,int right=256){int n=0;for(int y=0;y<256;++y)for(int x=left;x<right;++x)if(p[(y*256+x)*4]>20 || p[(y*256+x)*4+1]>20 || p[(y*256+x)*4+2]>20)++n;return n;}
}
int main() {
    auto root=std::filesystem::absolute("rpg-world-fixture");auto directory=root/"assets/world";std::filesystem::create_directories(directory);
    Fixture(directory/"rpgrocket.wcg",0xff30dc20u);
    HWND window=CreateWindowA("STATIC","Rocket world",WS_POPUP,0,0,256,256,nullptr,nullptr,nullptr,nullptr);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;format.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));RocketView renderer;WorldLabelView view;view.active=true;view.eye[2]=96;
    for(int cycle=0;cycle<2;++cycle) {
        HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));glViewport(0,0,256,256);renderer.Reset(false);
        RocketVisual rocket;rocket.position={100,-20,96};rocket.direction={1,0,0};rocket.scale=1;
        Clear();renderer.Draw(root.string(),{rocket},false,{},view,256,256,8,5000);auto body=Pixels();assert(Ink(body)>50);
        Clear(0);renderer.Draw(root.string(),{rocket},false,{},view,256,256,8,5000);assert(Ink(Pixels())==0);
        rocket.scale=2;Clear();renderer.Draw(root.string(),{rocket},false,{},view,256,256,8,5000);assert(Ink(Pixels())>Ink(body)*2);
        rocket.powered=true;rocket.trail={{100,0,96},{100,20,96}};Clear();renderer.Draw(root.string(),{rocket},false,{},view,256,256,8,5000);assert(Ink(Pixels())>Ink(body));
        Clear();renderer.Draw(root.string(),{},true,{100,0,96},view,256,256,8,5000);assert(Ink(Pixels())>5);
        // Both laser and airborne explosion stay behind native depth, with their original procedural coverage.
        rocket.explosion=true;rocket.age=.1f;rocket.scale=40;Clear();renderer.Draw(root.string(),{rocket},true,{100,0,96},view,256,256,8,5000);assert(Ink(Pixels())>50);
        Clear(0);renderer.Draw(root.string(),{rocket},true,{100,0,96},view,256,256,8,5000);assert(Ink(Pixels())==0);
        Clear();glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glEnable(GL_COLOR_LOGIC_OP);glLogicOp(GL_NOOP);glColorMask(0,0,0,0);glPixelStorei(GL_UNPACK_ROW_LENGTH,17);
        renderer.Draw(root.string(),{rocket},true,{100,0,96},view,256,256,8,5000);assert(Ink(Pixels())>50);
        GLint stride=0;glGetIntegerv(GL_UNPACK_ROW_LENGTH,&stride);assert(stride==17);
        GLboolean mask[4]={};glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && glIsEnabled(GL_COLOR_LOGIC_OP));assert(renderer.Valid());
        assert(glGetError()==GL_NO_ERROR);renderer.Reset(false);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    ReleaseDC(window,dc);DestroyWindow(window);std::filesystem::remove(directory/"rpgrocket.wcg");std::filesystem::remove(directory);std::filesystem::remove(root/"assets");std::filesystem::remove(root);
    std::puts("PASS WGL RPG body/trail/explosion/laser, depth, scaling, inherited GL state and replacement contexts.");
}
