#pragma once
#include <windows.h>
#include <gl/GL.h>
#include <d3d9.h>
// Adapter for the immediate drawing operations shared by current weapon, HUD and world-effect views.
// Outside BeginFrame/EndFrame every presentation call continues through the original OpenGL API.
namespace D3DGraphics {
bool Active();
bool BeginFrame(IDirect3DDevice9* device,HDC dc,int width,int height);
void EndFrame();
// Borrow a native render-target texture for one HUD quad; ownership stays with the capture adapter.
void DrawTexture(IDirect3DTexture9* texture,int x,int y,int width,int height);
void APIENTRY Begin(GLenum mode);
void APIENTRY End();
void APIENTRY Vertex3f(GLfloat x,GLfloat y,GLfloat z);
void APIENTRY Vertex2f(GLfloat x,GLfloat y);
void APIENTRY Vertex2i(GLint x,GLint y);
void APIENTRY TexCoord2f(GLfloat u,GLfloat v);
void APIENTRY TexCoord3f(GLfloat u,GLfloat v,GLfloat w);
void APIENTRY Color4f(GLfloat r,GLfloat g,GLfloat b,GLfloat a);
void APIENTRY Color3f(GLfloat r,GLfloat g,GLfloat b);
void APIENTRY Enable(GLenum capability);
void APIENTRY Disable(GLenum capability);
GLboolean APIENTRY IsEnabled(GLenum capability);
void APIENTRY DepthMask(GLboolean value);
void APIENTRY DepthFunc(GLenum value);
void APIENTRY DepthRange(GLclampd nearPlane,GLclampd farPlane);
void APIENTRY ClearDepth(GLclampd value);
void APIENTRY Clear(GLbitfield mask);
void APIENTRY AlphaFunc(GLenum function,GLclampf reference);
void APIENTRY BlendFunc(GLenum source,GLenum destination);
void APIENTRY ColorMask(GLboolean r,GLboolean g,GLboolean b,GLboolean a);
void APIENTRY LineWidth(GLfloat value);
void APIENTRY PolygonMode(GLenum,GLenum);
void APIENTRY Viewport(GLint x,GLint y,GLsizei width,GLsizei height);
void APIENTRY PushAttrib(GLbitfield);
void APIENTRY PopAttrib();
void APIENTRY PushClientAttrib(GLbitfield);
void APIENTRY PopClientAttrib();
void APIENTRY PixelStorei(GLenum,GLint);
void APIENTRY PixelTransferi(GLenum,GLint);
void APIENTRY PixelTransferf(GLenum,GLfloat);
void APIENTRY TexEnvi(GLenum,GLenum,GLint);
void APIENTRY MatrixMode(GLenum mode);
void APIENTRY LoadIdentity();
void APIENTRY LoadMatrixf(const GLfloat* value);
void APIENTRY PushMatrix();
void APIENTRY PopMatrix();
void APIENTRY Translatef(GLfloat x,GLfloat y,GLfloat z);
void APIENTRY Scalef(GLfloat x,GLfloat y,GLfloat z);
void APIENTRY Rotatef(GLfloat angle,GLfloat x,GLfloat y,GLfloat z);
void APIENTRY Ortho(GLdouble l,GLdouble r,GLdouble b,GLdouble t,GLdouble n,GLdouble f);
void APIENTRY Frustum(GLdouble l,GLdouble r,GLdouble b,GLdouble t,GLdouble n,GLdouble f);
void APIENTRY GetIntegerv(GLenum name,GLint* result);
void APIENTRY GetFloatv(GLenum name,GLfloat* result);
void APIENTRY GetBooleanv(GLenum name,GLboolean* result);
void APIENTRY GetTexEnviv(GLenum,GLenum,GLint* value);
GLenum APIENTRY GetError();
const GLubyte* APIENTRY GetString(GLenum name);
HGLRC WINAPI GetCurrentContext();
HDC WINAPI GetCurrentDC();
PROC WINAPI GetProcAddress(LPCSTR);
void APIENTRY GenTextures(GLsizei count,GLuint* names);
void APIENTRY BindTexture(GLenum target,GLuint name);
void APIENTRY DeleteTextures(GLsizei count,const GLuint* names);
GLboolean APIENTRY IsTexture(GLuint name);
void APIENTRY TexParameteri(GLenum target,GLenum name,GLint value);
void APIENTRY TexImage2D(GLenum target,GLint level,GLint internal,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels);
void APIENTRY GetTexLevelParameteriv(GLenum target,GLint,GLenum name,GLint* value);
}
