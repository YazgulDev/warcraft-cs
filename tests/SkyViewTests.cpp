#include "../src/presentation/SkyView.hpp"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <cstring>
namespace wc3 { void Log(const char*,...) {} }

int main() {
    char working[MAX_PATH];GetCurrentDirectoryA(MAX_PATH,working);
    std::string root=std::string(working)+"\\sky-test";
    std::filesystem::create_directories(root+"\\assets\\skies");
    const unsigned char colors[][4]={{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255},{255,0,255,255},{0,255,255,255}};
    { std::ofstream file(root+"\\assets\\skies\\test.wcs",std::ios::binary);unsigned size=4;
      file.write("WCS1",4);file.write(reinterpret_cast<const char*>(&size),4);file.write(reinterpret_cast<const char*>(&size),4);
      for (auto& color:colors) for (int pixel=0;pixel<16;++pixel) file.write(reinterpret_cast<const char*>(color),4); }
    { std::ofstream file(root+"\\assets\\skies\\gradient.wcs",std::ios::binary);unsigned size=16;
      file.write("WCS1",4);file.write(reinterpret_cast<const char*>(&size),4);file.write(reinterpret_cast<const char*>(&size),4);
      // Top-left caches brighten upward on all four horizon faces, catching an incorrect Z-up/Y-up basis.
      for (int face=0;face<6;++face) for (int y=0;y<16;++y) for (int x=0;x<16;++x) {
          unsigned char color[]={static_cast<unsigned char>(255-y*17),0,0,255};file.write(reinterpret_cast<const char*>(color),4);
      } }
    HWND window=CreateWindowA("STATIC","Private sky test",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);
    assert(window);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;format.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));
    SkyView sky;
    // An actual GL oracle reproduces Warcraft's inherited unpack state across three context replacements.
    for (int cycle=0;cycle<3;++cycle) {
        HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));glViewport(0,0,64,64);
        glPixelStorei(GL_UNPACK_ROW_LENGTH,512);glPixelStorei(GL_UNPACK_SKIP_PIXELS,32);glPixelStorei(GL_UNPACK_SKIP_ROWS,1024);
        const float angles[][2]={{0,0},{180,0},{90,0},{270,0},{0,89},{0,-89}};
        const int expected[]={0,1,3,2,4,5}; // GoldSrc's front/back sides join rt/lf at the proper edges.
        for (int face=0;face<6;++face) {
            glClearDepth(1);glClearColor(0,0,0,1);glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
            sky.Draw(root,"test",angles[face][0],angles[face][1],85,1);
            unsigned char color[4]={};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,color);
            assert(!memcmp(color,colors[expected[face]],3));
        }
        GLint stride=0;glGetIntegerv(GL_UNPACK_ROW_LENGTH,&stride);assert(stride==512);
        glGetIntegerv(GL_UNPACK_SKIP_ROWS,&stride);assert(stride==1024);
        for (float yaw:{0.0f,90.0f,180.0f,270.0f}) {
            glClearDepth(1);glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);sky.Draw(root,"gradient",yaw,0,85,1);
            unsigned char top[4],bottom[4];glReadPixels(32,48,1,1,GL_RGBA,GL_UNSIGNED_BYTE,top);glReadPixels(32,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,bottom);
            assert(top[0]>bottom[0]+80);
        }
        // Far-depth sky must not overwrite native foreground pixels.
        glClearDepth(.2);glClearColor(.25f,.5f,.75f,1);glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
        sky.Draw(root,"test",0,0,85,1);unsigned char foreground[4];glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,foreground);
        assert(foreground[0]>=63 && foreground[0]<=64 && foreground[1]>=127 && foreground[1]<=128);
        sky.Reset(false);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    ReleaseDC(window,dc);DestroyWindow(window);
}
