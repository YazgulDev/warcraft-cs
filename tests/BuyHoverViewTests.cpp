#include "../src/presentation/BuyMenuView.hpp"
#include "../src/presentation/BuyMenuLayout.hpp"
#include "../src/presentation/HudFont.hpp"
#include "../src/presentation/HudRenderState.hpp"
#include "../src/presentation/ItemNameView.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>

namespace wc3 {void Log(const char*,...) {}}
namespace {
constexpr int width=1024,height=768;
std::vector<unsigned char> Pixels(){std::vector<unsigned char> p(width*height*4);glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;}
void Clear(){glColorMask(1,1,1,1);glClearColor(.025f,.035f,.045f,1);glClear(GL_COLOR_BUFFER_BIT);}
int Ink(const std::vector<unsigned char>& p){int n=0;for(int y=0;y<height;++y)for(int x=100;x<width-100;++x)if(p[(size_t(y)*width+x)*4]>60)++n;return n;}
void Save(const char* name){auto p=Pixels();for(size_t i=0;i<p.size();i+=4)std::swap(p[i],p[i+2]);BITMAPFILEHEADER f={};BITMAPINFOHEADER b={};
    f.bfType=0x4D42;f.bfOffBits=sizeof(f)+sizeof(b);f.bfSize=f.bfOffBits+unsigned(p.size());b.biSize=sizeof(b);b.biWidth=width;b.biHeight=height;b.biPlanes=1;b.biBitCount=32;
    std::ofstream out(name,std::ios::binary);out.write(reinterpret_cast<char*>(&f),sizeof(f));out.write(reinterpret_cast<char*>(&b),sizeof(b));out.write(reinterpret_cast<char*>(p.data()),p.size());}
}
int main(){
    _set_error_mode(_OUT_TO_STDERR);
    HWND window=CreateWindowA("STATIC","Buy and rune HUD regression",WS_POPUP,0,0,width,height,nullptr,nullptr,nullptr,nullptr);assert(window);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));ItemNameView caption;HudFont font;
    for(int cycle=0;cycle<2;++cycle){
        HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));glViewport(0,0,width,height);
        caption.Reset(false);font.Reset(false);assert(font.Ensure(17));
        // Unicode captions must survive a new graphics context and inherited host pixel-upload state.
        Clear();caption.Draw(u8"\u0420\u0443\u043d\u0430 \u0438\u0441\u0446\u0435\u043b\u0435\u043d\u0438\u044f",width,height,24);auto expected=Pixels();
        assert(caption.Valid() && Ink(expected)>100);caption.Reset(true);
        glPixelStorei(GL_UNPACK_ALIGNMENT,8);glPixelStorei(GL_UNPACK_ROW_LENGTH,1024);glPixelStorei(GL_UNPACK_SKIP_ROWS,512);glPixelStorei(GL_UNPACK_SKIP_PIXELS,31);
        glPixelTransferi(GL_MAP_COLOR,GL_TRUE);glPixelTransferf(GL_ALPHA_SCALE,0);glPixelTransferf(GL_ALPHA_BIAS,1);
        Clear();glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glColorMask(0,0,0,0);glMatrixMode(GL_TEXTURE);glLoadIdentity();glTranslatef(.7f,.2f,0);glMatrixMode(GL_PROJECTION);
        caption.Draw(u8"\u0420\u0443\u043d\u0430 \u0438\u0441\u0446\u0435\u043b\u0435\u043d\u0438\u044f",width,height,24);
        GLint value=0;glGetIntegerv(GL_UNPACK_SKIP_ROWS,&value);assert(value==512);
        GLfloat scale=0;glGetFloatv(GL_ALPHA_SCALE,&scale);assert(scale==0);
        glGetIntegerv(GL_MATRIX_MODE,&value);assert(value==GL_PROJECTION);
        glPushAttrib(GL_PIXEL_MODE_BIT);glPixelTransferi(GL_MAP_COLOR,GL_FALSE);glPixelTransferf(GL_ALPHA_SCALE,1);glPixelTransferf(GL_ALPHA_BIAS,0);
        assert(Pixels()==expected);glPopAttrib();
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(1,1,1,1);glMatrixMode(GL_TEXTURE);glLoadIdentity();
        glPixelTransferi(GL_MAP_COLOR,GL_FALSE);glPixelTransferf(GL_ALPHA_SCALE,1);glPixelTransferf(GL_ALPHA_BIAS,0);
        glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
        if(!cycle){
            Clear();caption.Draw(u8"\u0420\u0443\u043d\u0430 \u0438\u0441\u0446\u0435\u043b\u0435\u043d\u0438\u044f",width,height,24);Save("rune-caption-preview.bmp");
            GameplaySettings settings;BuyMenuState menu(settings);menu.page=BuyMenu::Page::Main;menu.dollars=800;menu.gold=4500;
            auto draw=[&](const char* file){Clear();HudRenderState scope;glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,width,height,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
                auto text=[&](float x,float y,const char* label){font.Draw(x,y,label);};
                BuyMenuView::Draw(menu,width,height,POINT{5,5},text,text,[](int,float,float,float,float){});Save(file);};
            draw("buy-dollars-preview.bmp");menu.currency=EconomySettings::Currency::Gold;menu.page=BuyMenu::Page::Rifles;draw("buy-gold-preview.bmp");
            BuyMenuLayout box(width,height);assert(BuyMenuLayout::Key(int(box.listX+10),int(box.paymentY+10),width,height,menu.page,&settings)==BuyMenu::PayDollars);
            assert(BuyMenuLayout::Key(int(box.detailX+10),int(box.paymentY+10),width,height,menu.page,&settings)==BuyMenu::PayGold);
        }
        caption.Reset(true);font.Reset(true);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    ReleaseDC(window,dc);DestroyWindow(window);std::puts("Actual buy panel and localized rune pixels, host render/upload state and graphics-context replacement passed.");
}
