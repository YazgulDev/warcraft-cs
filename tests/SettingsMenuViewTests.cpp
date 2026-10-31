#include "../src/presentation/SettingsMenuView.hpp"
#include "../src/presentation/SettingsMenuLayout.hpp"
#include "../src/presentation/HudFont.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <fstream>
namespace wc3 {void Log(const char*,...) {}}
int main() {
    _set_error_mode(_OUT_TO_STDERR);
    HWND window=CreateWindowA("STATIC","Settings pixels",WS_POPUP,0,0,1024,768,nullptr,nullptr,nullptr,nullptr);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR pf={};pf.nSize=sizeof(pf);pf.nVersion=1;pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=32;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&pf),&pf));HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));
    HudFont font;SettingsMenu menu;GameplaySettings settings;menu.Queue({SettingsMenu::Kind::Toggle});
    menu.Update(settings,'A',[]{return std::vector<std::string>{"Des","snow"};},[](const auto&){return true;});
    for(auto size:{std::pair<int,int>{800,600},{1024,768}}) {
        int w=size.first,h=size.second;glViewport(0,0,w,h);assert(font.Ensure(h/38));
        glClearColor(0,0,0,1);glColorMask(1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glColorMask(0,0,0,0);glEnable(GL_COLOR_LOGIC_OP);glLogicOp(GL_NOOP);
        SettingsMenuView::Draw(menu,w,h,w/2,h/2,[&](float x,float y,const char* label){font.Draw(x,y,label);});
        std::vector<unsigned char> pixels(size_t(w)*h*4);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        int amber=0,white=0;for(size_t i=0;i<pixels.size();i+=4){amber+=pixels[i]>140 && pixels[i+1]>60 && pixels[i+2]<120;white+=pixels[i]>180 && pixels[i+1]>180;}
        assert(amber>1000 && white>1000);GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && glIsEnabled(GL_COLOR_LOGIC_OP));
        if(w==1024) {
            // Store a private regression screenshot without any game-derived textures.
            for(size_t i=0;i<pixels.size();i+=4)std::swap(pixels[i],pixels[i+2]);
            BITMAPFILEHEADER f={};BITMAPINFOHEADER b={};f.bfType=0x4D42;f.bfOffBits=sizeof(f)+sizeof(b);f.bfSize=f.bfOffBits+unsigned(pixels.size());
            b.biSize=sizeof(b);b.biWidth=w;b.biHeight=h;b.biPlanes=1;b.biBitCount=32;
            std::ofstream image("settings-menu-ui.bmp",std::ios::binary);image.write(reinterpret_cast<char*>(&f),sizeof(f));image.write(reinterpret_cast<char*>(&b),sizeof(b));image.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
        }
    }
    font.Reset(true);assert(glGetError()==GL_NO_ERROR);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
    std::puts("PASS actual settings panel/slider/font pixels, two sizes, inherited raster isolation and full state restoration.");
}
