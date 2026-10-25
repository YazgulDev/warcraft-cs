#include "../src/presentation/GunMesh.hpp"
#include "../src/presentation/MuzzleFlashView.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

namespace wc3 {void Log(const char*,...) {}}
namespace {
void Require(bool value,const char* reason) {if(!value){std::fprintf(stderr,"%s\n",reason);std::exit(1);}}
template<class T> void Write(std::ofstream& file,const T& value) {file.write(reinterpret_cast<const char*>(&value),sizeof(value));}
void Fixture(const char* path,int footer) {
    std::ofstream file(path,std::ios::binary);
    for(unsigned value:{0x32474357u,1u,1u,1u,1u,1u,1u,0xffffffffu,0u,3u})Write(file,value);
    struct Vertex {float x,y,z,u,v;unsigned bone;};
    for(Vertex v: {Vertex{20,8,0,0,0,0},Vertex{10,4,0,0,0,0},Vertex{10,8,4,0,0,0}})Write(file,v);
    char name[32]="idle";file.write(name,32);Write(file,1.f);Write(file,2u);
    // The second bone frame shifts the barrel four units right; halfway sampling must shift it two.
    for(float shift:{0.f,4.f})for(float value:{1.f,0.f,0.f,0.f,0.f,1.f,0.f,shift,0.f,0.f,1.f,0.f})Write(file,value);
    if(footer>=0){Write(file,0x314D4357u);Write(file,1u);Write(file,unsigned(footer));Write(file,20.f);Write(file,8.f);Write(file,0.f);}
}
void Clear() {glColorMask(1,1,1,1);glDepthMask(1);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}
std::vector<unsigned char> Pixels() {std::vector<unsigned char> p(256*256*4);glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;}
int Ink(const std::vector<unsigned char>& p,int left=0,int bottom=0,int right=256,int top=256) {
    int n=0;for(int y=bottom;y<top;++y)for(int x=left;x<right;++x)if(p[(y*256+x)*4]>30)++n;return n;
}
}
int main() {
    HWND window=CreateWindowA("STATIC","Muzzle regression",WS_POPUP,0,0,256,256,nullptr,nullptr,nullptr,nullptr);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;format.cDepthBits=24;
    Require(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format)!=FALSE,"WGL format failed");
    HGLRC context=wglCreateContext(dc);Require(context && wglMakeCurrent(dc,context),"WGL context failed");
    glViewport(0,0,256,256);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(-32,32,-32,32,1,200);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    Fixture("muzzle-authored.wcg",0);Fixture("muzzle-legacy.wcg",-1);Fixture("muzzle-invalid.wcg",1);
    GunMesh mesh;Require(mesh.Load("muzzle-authored.wcg"),"Authored cache failed");
    std::array<float,3> anchor;
    Require(mesh.MuzzlePoint("idle",.5f,1,anchor) && anchor[0]==10 && anchor[1]==0 && anchor[2]==-20,
        "Beam attachment did not share the flash's interpolated pose");
    Clear();mesh.Draw("idle",0,0,1,1,false);Require(Ink(Pixels())==0,"Idle emitted a flash");
    mesh.Draw("idle",0,1,1,1,false);auto first=Pixels();
    Require(Ink(first)>15 && Ink(first,148,116,172,140)==Ink(first),"Authored barrel anchor projected incorrectly");
    Clear();mesh.Draw("idle",.5f,1,1,2,false);
    Require(Ink(Pixels(),166,124,170,132)>0,"Flash did not follow interpolated animation bone");
    Clear();glPushMatrix();glTranslatef(4,-4,0);mesh.Draw("idle",0,1,1,3,false);glPopMatrix();
    Require(Ink(Pixels(),172,108,180,116)>0,"Flash did not follow shared weapon movement");
    Require(mesh.Load("muzzle-legacy.wcg"),"Legacy cache failed");Clear();mesh.Draw("idle",0,1,1,4,false);
    Require(Ink(Pixels())>15,"Legacy geometry anchor missing");
    Require(!mesh.Load("muzzle-invalid.wcg"),"Invalid footer bone accepted");mesh.Reset(true);
    // Warcraft may leave wireframe, write masks and logic ops active. Restore each after an independent flash.
    Clear();glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glEnable(GL_POLYGON_STIPPLE);glEnable(GL_COLOR_LOGIC_OP);glLogicOp(GL_NOOP);glColorMask(0,0,0,0);
    MuzzleFlashView::Draw(8,0,-20,1,1,1);Require(Pixels()==first,"Host render state changed flash pixels");
    GLint polygon[2]={};GLboolean mask[4]={};glGetIntegerv(GL_POLYGON_MODE,polygon);glGetBooleanv(GL_COLOR_WRITEMASK,mask);
    Require(polygon[0]==GL_LINE && !mask[0] && glIsEnabled(GL_COLOR_LOGIC_OP) && glIsEnabled(GL_POLYGON_STIPPLE),"Host render state leaked");
    float depth=0;glReadPixels(160,128,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&depth);Require(depth>.99f,"Flash wrote into depth buffer");
    Require(glGetError()==GL_NO_ERROR,"Flash generated GL error");
    wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
    for(const char* path:{"muzzle-authored.wcg","muzzle-legacy.wcg","muzzle-invalid.wcg"})std::remove(path);
    std::puts("PASS actual WGL flash pixels, animated/legacy anchors, shared motion and host-state restoration.");
}
