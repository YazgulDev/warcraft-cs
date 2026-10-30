#include "../src/presentation/HudFont.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fstream>

namespace {
constexpr int width=640,height=240;
void Require(bool value,const char* reason) {
    if(!value) {std::fprintf(stderr,"%s\n",reason);std::exit(1);}
}
std::vector<unsigned char> Pixels() {
    std::vector<unsigned char> pixels(width*height*4);
    // Readback has its own pixel-transfer stage; observe the framebuffer, not the poisoned host mapping.
    glPushAttrib(GL_PIXEL_MODE_BIT);glPixelTransferi(GL_MAP_COLOR,GL_FALSE);
    glPixelTransferf(GL_ALPHA_SCALE,1);glPixelTransferf(GL_ALPHA_BIAS,0);
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());glPopAttrib();return pixels;
}
void Clear() {
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glColor4f(1,.85f,.3f,1);
}
int Coverage(const std::vector<unsigned char>& pixels,int left,int top,int right,int bottom) {
    int count=0;
    for(int y=top;y<bottom;++y)for(int x=left;x<right;++x)
        if(pixels[(size_t(height-1-y)*width+x)*4]>30)++count;
    return count;
}
void PoisonUpload() {
    glPixelStorei(GL_UNPACK_ALIGNMENT,8);glPixelStorei(GL_UNPACK_ROW_LENGTH,1024);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,512);glPixelStorei(GL_UNPACK_SKIP_PIXELS,31);
    glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_TRUE);glPixelStorei(GL_UNPACK_LSB_FIRST,GL_TRUE);
    glPixelTransferi(GL_MAP_COLOR,GL_TRUE);glPixelTransferf(GL_ALPHA_SCALE,0);glPixelTransferf(GL_ALPHA_BIAS,1);
}
void CheckUploadState() {
    GLint value=0;GLfloat scalar=0;
    glGetIntegerv(GL_UNPACK_ROW_LENGTH,&value);Require(value==1024,"Lost host unpack stride");
    glGetIntegerv(GL_UNPACK_SKIP_ROWS,&value);Require(value==512,"Lost host unpack row offset");
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&value);Require(value==31,"Lost host unpack column offset");
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&value);Require(value==8,"Lost host unpack alignment");
    glGetIntegerv(GL_UNPACK_SWAP_BYTES,&value);Require(value==GL_TRUE,"Lost byte swapping");
    glGetIntegerv(GL_UNPACK_LSB_FIRST,&value);Require(value==GL_TRUE,"Lost bit order");
    glGetIntegerv(GL_MAP_COLOR,&value);Require(value==GL_TRUE,"Lost host color mapping");
    glGetFloatv(GL_ALPHA_SCALE,&scalar);Require(scalar==0,"Lost host alpha scale");
    glGetFloatv(GL_ALPHA_BIAS,&scalar);Require(scalar==1,"Lost host alpha bias");
}
void SavePreview(const std::vector<unsigned char>& pixels) {
    BITMAPFILEHEADER file={};BITMAPINFOHEADER info={};
    file.bfType=0x4D42;file.bfOffBits=sizeof(file)+sizeof(info);file.bfSize=file.bfOffBits+width*height*4;
    info.biSize=sizeof(info);info.biWidth=width;info.biHeight=height;info.biPlanes=1;info.biBitCount=32;
    auto bgra=pixels;
    for(size_t i=0;i<bgra.size();i+=4) {auto red=bgra[i];bgra[i]=bgra[i+2];bgra[i+2]=red;}
    std::ofstream output("hud-font-preview.bmp",std::ios::binary);
    output.write(reinterpret_cast<const char*>(&file),sizeof(file));output.write(reinterpret_cast<const char*>(&info),sizeof(info));
    output.write(reinterpret_cast<const char*>(bgra.data()),bgra.size());
}
}

int main() {
    HWND window=CreateWindowA("STATIC","HUD font regression",WS_POPUP,0,0,width,height,nullptr,nullptr,nullptr,nullptr);
    Require(window!=nullptr,"Cannot create hidden test window");HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;
    Require(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format)!=FALSE,"Cannot set WGL format");
    HudFont font;
    for(int cycle=0;cycle<3;++cycle) {
        HGLRC context=wglCreateContext(dc);Require(context && wglMakeCurrent(dc,context),"Cannot create context");
        std::printf("HUD font context %d: %s\n",cycle,glGetString(GL_RENDERER));
        glViewport(0,0,width,height);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,width,height,0,-1,1);
        glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        // Forgetting an old context must never delete an unrelated texture with a recycled numeric ID.
        GLuint unrelated=0;glGenTextures(1,&unrelated);glBindTexture(GL_TEXTURE_2D,unrelated);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        font.Reset(false);Require(glIsTexture(unrelated)==GL_TRUE,"Reset deleted the new context's texture");
        for(int size : {12,20,26,42,84}) {
            font.Reset(true);
            glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
            glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
            glPixelTransferi(GL_MAP_COLOR,GL_FALSE);glPixelTransferf(GL_ALPHA_SCALE,1);glPixelTransferf(GL_ALPHA_BIAS,0);
            Require(font.Ensure(size) && font.Valid(),"Cannot build normal font");
            Clear();font.Draw(10,100,"O O");auto expected=Pixels();
            int advance=font.Advance();
            int ink=Coverage(expected,10,100-size,10+advance,100);
            Require(ink>4 && ink<advance*size/2,"Glyph must have visible ink and transparent counters");
            Require(Coverage(expected,10+advance,100-size,10+advance*2,100)==0,"Space became an opaque rectangle");
            Require(Coverage(expected,10+advance*2,100-size,10+advance*3,100)==ink,"Glyph advances are inconsistent");
            Clear();font.Draw(10,100,"\xD0\x96 \xD0\x96");auto cyrillic=Pixels();
            Require(Coverage(cyrillic,10,100-size,10+advance,100)>4,"Cyrillic glyph has no visible pixels");
            Clear();font.Draw(10,100,"? ?");Require(Pixels()!=cyrillic,"Cyrillic text fell back to question marks");
            font.Reset(true);PoisonUpload();Require(font.Ensure(size),"Cannot build font with inherited pixel state");
            CheckUploadState();Clear();font.Draw(10,100,"O O");
            Require(Pixels()==expected,"Inherited upload state changed glyph coverage");
            // Wireframe, logic ops, a texture transform and masks cannot fill or hide the letters.
            Clear();unsigned char empty[128]={};
            glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
            glPolygonStipple(empty);glEnable(GL_POLYGON_STIPPLE);glEnable(GL_COLOR_LOGIC_OP);glLogicOp(GL_NOOP);
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
            glMatrixMode(GL_TEXTURE);glTranslatef(.75f,.5f,0);glMatrixMode(GL_PROJECTION);
            font.Draw(10,100,"O O");Require(Pixels()==expected,"Inherited draw state changed glyph coverage");
            GLint mode=0,modes[2]={};GLboolean mask[4]={};
            glGetIntegerv(GL_MATRIX_MODE,&mode);glGetIntegerv(GL_POLYGON_MODE,modes);glGetBooleanv(GL_COLOR_WRITEMASK,mask);
            Require(mode==GL_PROJECTION && modes[0]==GL_LINE && !mask[0] && !mask[3] &&
                glIsEnabled(GL_POLYGON_STIPPLE) && glIsEnabled(GL_COLOR_LOGIC_OP),"Font failed to restore host draw state");
            GLfloat matrix[16];glGetFloatv(GL_TEXTURE_MATRIX,matrix);
            Require(matrix[12]==.75f && matrix[13]==.5f,"Lost host texture transform");
            glMatrixMode(GL_TEXTURE);glLoadIdentity();glMatrixMode(GL_MODELVIEW);
            glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
            Clear();font.Draw(float(-advance),100,"O O O");Require(Coverage(Pixels(),0,0,width,height)>0,"Offscreen origin hid the entire string");
            Require(glGetError()==GL_NO_ERROR,"Font emitted an OpenGL error");
        }
        font.Reset(true);Require(font.Ensure(20),"Cannot create preview hints");Clear();
        font.Draw(20,35,"Warcraft CS by Yazgul | F6: RTS | F7: THIRD PERSON");
        font.Draw(20,65,"WASD: MOVE | B: BUY | 1234: SPELLS | M: MINIMAP");
        Require(font.Ensure(42),"Cannot resize status font");font.Draw(20,145,"HP 100   AK47   30 / 90");
        Require(font.Ensure(16),"Cannot resize spell font");font.Draw(20,200,"Z READY     X 12.5s     C READY     V 3s");
        if(cycle==2)SavePreview(Pixels());
        Require(glGetError()==GL_NO_ERROR,"Preview emitted an OpenGL error");
        wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    font.Reset(false);ReleaseDC(window,dc);DestroyWindow(window);
    std::puts("HUD text coverage, transparent spaces, 5 sizes, resize, inherited GL state and 3 context replacements passed.");
}
