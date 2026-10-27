#include "../src/presentation/HudDiagnostics.hpp"
#include "../src/presentation/HudRenderState.hpp"
#include "../src/presentation/HudFont.hpp"
#include "../src/platform/DiagnosticLog.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>

int main(int argc,char** argv) {
    assert(argc==2);LoggingSettings settings;settings.intervalMs=60000;
    DiagnosticLog::Configure(settings);DiagnosticLog::Open(argv[1]);
    HWND window=CreateWindowA("STATIC","HUD log regression",WS_POPUP,0,0,320,240,nullptr,nullptr,nullptr,nullptr);assert(window);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));
    HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));
    using Active=void (APIENTRY*)(GLenum);using Equation=void (APIENTRY*)(GLenum);
    auto active=reinterpret_cast<Active>(wglGetProcAddress("glActiveTexture"));
    auto equation=reinterpret_cast<Equation>(wglGetProcAddress("glBlendEquation"));assert(active && equation);
    active(0x84C1);glEnable(GL_TEXTURE_2D);glMatrixMode(GL_TEXTURE);glTranslatef(.25f,.5f,0);glMatrixMode(GL_PROJECTION);
    equation(0x800B);glColorMask(0,1,0,1);assert(glGetError()==GL_NO_ERROR);
    HudDiagnostics diagnostics;
    // A pre-existing host error must remain available to the normal error sampler after observational logging.
    glEnable(0xDEAD);bool sampled=diagnostics.Begin("weapons",320,240);assert(sampled);
    HudDiagnostics::State("after-weapon",sampled);
    GLint value=0;glGetIntegerv(0x84E0,&value);assert(value==0x84C1);
    GLfloat matrix[16];glGetFloatv(GL_TEXTURE_MATRIX,matrix);assert(matrix[12]==.25f && matrix[13]==.5f);
    GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && mask[1] && !mask[2] && mask[3]);
    {HudRenderState state;HudDiagnostics::State("prepared",sampled);}
    assert(glGetError()==GL_INVALID_ENUM && glGetError()==GL_NO_ERROR);
    assert(!diagnostics.Begin("weapons",320,240));assert(diagnostics.Begin("third-person",320,240));
    settings.detailed=false;DiagnosticLog::Configure(settings);assert(!diagnostics.Begin("clean-unit-view",320,240));
    // Failure/recovery events remain useful when per-frame TRACE is disabled, and repeated failures do not flood logs.
    HudFont font("test");assert(!font.Ensure(0));assert(!font.Ensure(0));assert(font.Ensure(18));font.Reset(true);
    assert(glGetError()==GL_NO_ERROR);DiagnosticLog::Close();
    std::ifstream input(std::string(argv[1])+"/WarcraftCS.log",std::ios::binary);
    std::string log{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
    assert(log.find("phase=host drawBuffer=")!=std::string::npos);
    // A zero current program no longer omits the independent program-pipeline binding from reports.
    assert(log.find("program=0 pipeline=0")!=std::string::npos);
    assert(log.find("blendEquation=800B/800B")!=std::string::npos && log.find("blendEquation=8006/8006")!=std::string::npos);
    assert(log.find("HUD texture phase=host unit=1 active=1")!=std::string::npos);
    auto first=log.find("HUD frame mode=weapons");assert(first!=std::string::npos && log.find("HUD frame mode=weapons",first+1)==std::string::npos);
    assert(log.find("HUD frame mode=third-person")!=std::string::npos && log.find("HUD frame mode=clean-unit-view")==std::string::npos);
    first=log.find("HUD font role=test phase=invalid-size");assert(first!=std::string::npos && log.find("HUD font role=test phase=invalid-size",first+1)==std::string::npos);
    assert(log.find("HUD font role=test phase=ready")!=std::string::npos && log.find("coverage=")!=std::string::npos);
    wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
    puts("PASS sampled HUD stages, raw/prepared state, caller-state/error preservation, TRACE opt-out and bounded font failure/recovery.");
}
