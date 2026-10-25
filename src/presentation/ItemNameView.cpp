#include "ItemNameView.hpp"
#include "HudRenderState.hpp"
#include <algorithm>
#include <vector>
#include <cstring>

namespace {
int PowerOfTwo(int value){int n=1;while(n<value)n*=2;return n;}
std::wstring Wide(const std::string& name) {
    // Warcraft maps normally use UTF-8; old localized strings retain the Windows code-page fallback.
    UINT page=CP_UTF8;int count=MultiByteToWideChar(page,MB_ERR_INVALID_CHARS,name.data(),int(name.size()),nullptr,0);
    if(!count){page=CP_ACP;count=MultiByteToWideChar(page,0,name.data(),int(name.size()),nullptr,0);}
    if(count<=0 || count>512)return {};
    std::wstring result(size_t(count),L'\0');MultiByteToWideChar(page,page==CP_UTF8 ? MB_ERR_INVALID_CHARS : 0,name.data(),int(name.size()),result.data(),count);
    return result;
}
}
bool ItemNameView::Build(const std::string& name,int fontSize,int maximumWidth) {
    auto caption=Wide(name);if(caption.empty())return false;
    HDC dc=CreateCompatibleDC(nullptr);if(!dc)return false;
    HFONT font=CreateFontW(fontSize,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    if(!font){DeleteDC(dc);return false;}auto oldFont=SelectObject(dc,font);
    SIZE measured={};bool ready=GetTextExtentPoint32W(dc,caption.data(),int(caption.size()),&measured)!=FALSE;
    GLint limit=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&limit);
    int width=std::clamp(int(measured.cx)+6,8,std::max(8,std::min(maximumWidth,int(limit)))),height=fontSize+8,tw=PowerOfTwo(width),th=PowerOfTwo(height);
    BITMAPINFO info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=tw;info.bmiHeader.biHeight=-th;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    HGDIOBJ oldBitmap=nullptr;std::vector<unsigned char> alpha;
    if(ready && bitmap && pixels) {
        oldBitmap=SelectObject(dc,bitmap);std::memset(pixels,0,size_t(tw)*th*4);SetBkColor(dc,RGB(0,0,0));SetTextColor(dc,RGB(255,255,255));SetBkMode(dc,OPAQUE);
        RECT box={3,0,width,height};DrawTextW(dc,caption.data(),int(caption.size()),&box,DT_SINGLELINE|DT_NOPREFIX|DT_END_ELLIPSIS);
        GdiFlush();alpha.resize(size_t(tw)*th);
        auto rgba=static_cast<const unsigned char*>(pixels);
        for(size_t i=0;i<alpha.size();++i)alpha[i]=std::max({rgba[i*4],rgba[i*4+1],rgba[i*4+2]});
    }
    if(oldBitmap)SelectObject(dc,oldBitmap);if(bitmap)DeleteObject(bitmap);SelectObject(dc,oldFont);DeleteObject(font);DeleteDC(dc);
    if(alpha.empty())return false;
    // Inherited upload offsets, color maps and alpha transfer must not replace letters with opaque rectangles.
    glPushAttrib(GL_TEXTURE_BIT|GL_PIXEL_MODE_BIT);glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_FALSE);glPixelStorei(GL_UNPACK_LSB_FIRST,GL_FALSE);glPixelTransferi(GL_MAP_COLOR,GL_FALSE);
    glPixelTransferf(GL_ALPHA_SCALE,1);glPixelTransferf(GL_ALPHA_BIAS,0);
    GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,tw,th,0,GL_ALPHA,GL_UNSIGNED_BYTE,alpha.data());GLint uploaded=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&uploaded);
    glPopClientAttrib();glPopAttrib();if(!texture || uploaded!=tw){if(texture)glDeleteTextures(1,&texture);return false;}
    Reset(true);texture_=texture;name_=name;fontSize_=fontSize;maximumWidth_=maximumWidth;width_=width;height_=height;textureWidth_=tw;textureHeight_=th;return true;
}
void ItemNameView::Draw(const std::string& name,int width,int height,int fontSize) {
    DrawAt(name,width,height,fontSize,width*.5f,height*.5f+fontSize*1.7f,1);
}
void ItemNameView::DrawAt(const std::string& name,int width,int height,int fontSize,float centerX,float top,float alpha,float depth) {
    if(name.empty() || width<32 || height<32)return;
    HudRenderState state;int maximum=std::min(1600,width*4/5);
    if((!texture_ || name!=name_ || fontSize!=fontSize_ || maximum!=maximumWidth_) && !Build(name,fontSize,maximum))return;
    // Text uses a copied world projection/depth, with the same scoped Unicode upload state as item names.
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture_);
    if(depth>=0){glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthRange(0,1);}
    glColor4f(1,depth>=0 ? .3f : .9f,depth>=0 ? .12f : .55f,alpha);
    float x=centerX-width_*.5f,y=top,u=float(width_)/textureWidth_,v=float(height_)/textureHeight_,z=depth>=0 ? 1-2*depth : 0;
    glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex3f(x,y,z);glTexCoord2f(u,0);glVertex3f(x+width_,y,z);
    glTexCoord2f(u,v);glVertex3f(x+width_,y+height_,z);glTexCoord2f(0,v);glVertex3f(x,y+height_,z);glEnd();
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();
}
void ItemNameView::Reset(bool deleteObjects) {
    if(deleteObjects && texture_)glDeleteTextures(1,&texture_);texture_=0;name_.clear();fontSize_=maximumWidth_=0;
}
