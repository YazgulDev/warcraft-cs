#include "GraphicsApi.hpp"
#include "HudFont.hpp"
#include "HudDiagnostics.hpp"
#include <cstring>
#include <algorithm>
#include <vector>

namespace {
int PowerOfTwo(int value) {
    int result=1;
    while(result<value)result*=2;
    return result;
}
}

bool HudFont::Report(const char* phase,int requested,int width,int height,unsigned texture,unsigned long error,int glyph,int coverage) {
    // Persistent creation failures are bounded per font; a changed failure or recovery is reported immediately.
    DWORD now=GetTickCount();auto context=wglGetCurrentContext();
    bool changed=std::strcmp(lastPhase_,phase)!=0 || context!=lastReportContext_;
    if(changed || !lastReport_ || now-lastReport_>=1000) {
        lastPhase_=phase;lastReport_=now;lastReportContext_=context;HudDiagnostics::Font(role_,phase,requested,width,height,texture,error,glyph,coverage);
    }
    return !std::strcmp(phase,"ready");
}

bool HudFont::Ensure(int height) {
    if(texture_ && height_==height)return true;
    if(height<1 || height>128)return Report("invalid-size",height);
    // A memory DC avoids GDI calls on Warcraft's double-buffered OpenGL window.
    HDC dc=CreateCompatibleDC(nullptr);
    if(!dc)return Report("create-dc",height,0,0,0,GetLastError());
    HFONT font=CreateFontA(height,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,
        OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FIXED_PITCH,"Consolas");
    if(!font) {DWORD error=GetLastError();DeleteDC(dc);return Report("create-font",height,0,0,0,error);}
    HGDIOBJ previous=SelectObject(dc,font);
    TEXTMETRICA metrics={};
    bool valid=previous && previous!=HGDI_ERROR && GetTextMetricsA(dc,&metrics);
    DWORD error=valid ? 0 : GetLastError();int failedGlyph=-1;
    MAT2 transform={};transform.eM11.value=transform.eM22.value=1;
    std::array<Glyph,95> glyphs{};
    std::array<std::vector<unsigned char>,95> bitmaps;
    int cellWidth=1,cellHeight=1;
    for(unsigned i=0;valid && i<glyphs.size();++i) {
        GLYPHMETRICS shape={};
        DWORD bytes=GetGlyphOutlineA(dc,i+32,GGO_GRAY8_BITMAP,&shape,0,nullptr,&transform);
        if(bytes==GDI_ERROR) {valid=false;error=GetLastError();failedGlyph=int(i+32);break;}
        auto& glyph=glyphs[i];
        glyph.width=int(shape.gmBlackBoxX);glyph.height=int(shape.gmBlackBoxY);
        glyph.left=shape.gmptGlyphOrigin.x;glyph.top=shape.gmptGlyphOrigin.y;
        glyph.advance=shape.gmCellIncX;
        cellWidth=std::max(cellWidth,glyph.width+2);cellHeight=std::max(cellHeight,glyph.height+2);
        if(bytes) {
            bitmaps[i].resize(bytes);
            valid=GetGlyphOutlineA(dc,i+32,GGO_GRAY8_BITMAP,&shape,bytes,bitmaps[i].data(),&transform)!=GDI_ERROR;
            if(!valid){error=GetLastError();failedGlyph=int(i+32);}
        }
    }
    if(previous && previous!=HGDI_ERROR)SelectObject(dc,previous);
    DeleteObject(font);DeleteDC(dc);
    if(!valid)return Report(failedGlyph>=0 ? "glyph-outline" : "font-metrics",height,0,0,0,error,failedGlyph);
    int width=PowerOfTwo(cellWidth*16),atlasHeight=PowerOfTwo(cellHeight*6);
    GLint maximum=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum);
    if(width>maximum || atlasHeight>maximum)return Report("atlas-too-large",height,width,atlasHeight);
    std::vector<unsigned char> alpha(size_t(width)*atlasHeight,0);
    for(size_t i=0;i<glyphs.size();++i) {
        auto& glyph=glyphs[i];
        int x=int(i%16)*cellWidth+1,y=int(i/16)*cellHeight+1;
        int stride=(glyph.width+3)&~3;
        // GDI coverage is 0..64 with DWORD-aligned top-down rows; spaces stay transparent.
        for(int row=0;row<glyph.height && !bitmaps[i].empty();++row)
            for(int column=0;column<glyph.width;++column)
                alpha[size_t(y+row)*width+x+column]=static_cast<unsigned char>(
                    std::min(255,int(bitmaps[i][size_t(row)*stride+column])*255/64));
        glyph.u0=float(x)/width;glyph.v0=float(y)/atlasHeight;
        glyph.u1=float(x+glyph.width)/width;glyph.v1=float(y+glyph.height)/atlasHeight;
    }
    // Warcraft's upload stride and pixel-transfer state must not turn coverage into opaque rectangles.
    glPushAttrib(GL_TEXTURE_BIT|GL_PIXEL_MODE_BIT);
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_FALSE);glPixelStorei(GL_UNPACK_LSB_FIRST,GL_FALSE);
    glPixelTransferi(GL_MAP_COLOR,GL_FALSE);
    glPixelTransferf(GL_ALPHA_SCALE,1);glPixelTransferf(GL_ALPHA_BIAS,0);
    GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,width,atlasHeight,0,GL_ALPHA,GL_UNSIGNED_BYTE,alpha.data());
    GLint uploaded=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&uploaded);
    glPopClientAttrib();glPopAttrib();
    if(!texture || uploaded!=width) {if(texture)glDeleteTextures(1,&texture);return Report("texture-upload",height,width,atlasHeight,texture);}
    Reset(true);texture_=texture;height_=height;glyphs_=glyphs;advance_=std::max(1,int(metrics.tmAveCharWidth));
    // Coverage distinguishes a valid texture name from an atlas containing no readable glyph pixels.
    int ink=int(std::count_if(alpha.begin(),alpha.end(),[](unsigned char value){return value!=0;}));
    Report(ink ? "ready" : "empty-coverage",height,width,atlasHeight,texture_,0,-1,ink);
    return true;
}

void HudFont::Draw(float x,float baseline,const char* text) const {
    if(!texture_ || !text)return;
    // Text uses ordinary alpha-tested quads instead of driver-generated glBitmap display lists.
    glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_POLYGON_BIT|GL_CURRENT_BIT);
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture_);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glDisable(GL_TEXTURE_GEN_S);glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);glDisable(GL_TEXTURE_GEN_Q);
    glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);glAlphaFunc(GL_GREATER,0);
    GLint matrixMode=GL_MODELVIEW;glGetIntegerv(GL_MATRIX_MODE,&matrixMode);
    glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();
    glBegin(GL_QUADS);
    for(auto character=reinterpret_cast<const unsigned char*>(text);*character;++character) {
        const auto& glyph=glyphs_[*character>=32 && *character<=126 ? *character-32 : '?'-32];
        float left=x+glyph.left,top=baseline-glyph.top;
        if(glyph.width && glyph.height) {
            glTexCoord2f(glyph.u0,glyph.v0);glVertex2f(left,top);
            glTexCoord2f(glyph.u1,glyph.v0);glVertex2f(left+glyph.width,top);
            glTexCoord2f(glyph.u1,glyph.v1);glVertex2f(left+glyph.width,top+glyph.height);
            glTexCoord2f(glyph.u0,glyph.v1);glVertex2f(left,top+glyph.height);
        }
        x+=glyph.advance;
    }
    glEnd();glPopMatrix();glMatrixMode(matrixMode);glPopAttrib();
}

void HudFont::Reset(bool deleteObjects) {
    // Context replacement forgets numeric IDs without deleting a new context's unrelated textures.
    if(deleteObjects && texture_)glDeleteTextures(1,&texture_);
    texture_=0;height_=0;advance_=12;
}
