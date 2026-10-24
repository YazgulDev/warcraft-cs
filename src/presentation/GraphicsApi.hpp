#pragma once
#include <windows.h>
#include <gl/GL.h>
#ifdef WCS_D3D9

#include "D3DGraphics.hpp"
// Shared draw code uses an Adapter: legacy WGL is forwarded verbatim, D3D frames use owned fixed-function state.
namespace GraphicsApi {
inline void APIENTRY glBegin(GLenum mode) {
    if(D3DGraphics::Active()) return D3DGraphics::Begin(mode);
    return ::glBegin(mode);
}
inline void APIENTRY glEnd() {
    if(D3DGraphics::Active()) return D3DGraphics::End();
    return ::glEnd();
}
inline void APIENTRY glVertex3f(GLfloat x,GLfloat y,GLfloat z) {
    if(D3DGraphics::Active()) return D3DGraphics::Vertex3f(x,y,z);
    return ::glVertex3f(x,y,z);
}
inline void APIENTRY glVertex2f(GLfloat x,GLfloat y) {
    if(D3DGraphics::Active()) return D3DGraphics::Vertex2f(x,y);
    return ::glVertex2f(x,y);
}
inline void APIENTRY glVertex2i(GLint x,GLint y) {
    if(D3DGraphics::Active()) return D3DGraphics::Vertex2i(x,y);
    return ::glVertex2i(x,y);
}
inline void APIENTRY glTexCoord2f(GLfloat u,GLfloat v) {
    if(D3DGraphics::Active()) return D3DGraphics::TexCoord2f(u,v);
    return ::glTexCoord2f(u,v);
}
inline void APIENTRY glTexCoord3f(GLfloat u,GLfloat v,GLfloat w) {
    if(D3DGraphics::Active()) return D3DGraphics::TexCoord3f(u,v,w);
    return ::glTexCoord3f(u,v,w);
}
inline void APIENTRY glColor4f(GLfloat r,GLfloat g,GLfloat b,GLfloat a) {
    if(D3DGraphics::Active()) return D3DGraphics::Color4f(r,g,b,a);
    return ::glColor4f(r,g,b,a);
}
inline void APIENTRY glColor3f(GLfloat r,GLfloat g,GLfloat b) {
    if(D3DGraphics::Active()) return D3DGraphics::Color3f(r,g,b);
    return ::glColor3f(r,g,b);
}
inline void APIENTRY glEnable(GLenum capability) {
    if(D3DGraphics::Active()) return D3DGraphics::Enable(capability);
    return ::glEnable(capability);
}
inline void APIENTRY glDisable(GLenum capability) {
    if(D3DGraphics::Active()) return D3DGraphics::Disable(capability);
    return ::glDisable(capability);
}
inline GLboolean APIENTRY glIsEnabled(GLenum capability) {
    if(D3DGraphics::Active()) return D3DGraphics::IsEnabled(capability);
    return ::glIsEnabled(capability);
}
inline void APIENTRY glDepthMask(GLboolean value) {
    if(D3DGraphics::Active()) return D3DGraphics::DepthMask(value);
    return ::glDepthMask(value);
}
inline void APIENTRY glDepthFunc(GLenum value) {
    if(D3DGraphics::Active()) return D3DGraphics::DepthFunc(value);
    return ::glDepthFunc(value);
}
inline void APIENTRY glDepthRange(GLclampd nearPlane,GLclampd farPlane) {
    if(D3DGraphics::Active()) return D3DGraphics::DepthRange(nearPlane,farPlane);
    return ::glDepthRange(nearPlane,farPlane);
}
inline void APIENTRY glClearDepth(GLclampd value) {
    if(D3DGraphics::Active()) return D3DGraphics::ClearDepth(value);
    return ::glClearDepth(value);
}
inline void APIENTRY glClear(GLbitfield mask) {
    if(D3DGraphics::Active()) return D3DGraphics::Clear(mask);
    return ::glClear(mask);
}
inline void APIENTRY glAlphaFunc(GLenum function,GLclampf reference) {
    if(D3DGraphics::Active()) return D3DGraphics::AlphaFunc(function,reference);
    return ::glAlphaFunc(function,reference);
}
inline void APIENTRY glBlendFunc(GLenum source,GLenum destination) {
    if(D3DGraphics::Active()) return D3DGraphics::BlendFunc(source,destination);
    return ::glBlendFunc(source,destination);
}
inline void APIENTRY glColorMask(GLboolean r,GLboolean g,GLboolean b,GLboolean a) {
    if(D3DGraphics::Active()) return D3DGraphics::ColorMask(r,g,b,a);
    return ::glColorMask(r,g,b,a);
}
inline void APIENTRY glLineWidth(GLfloat value) {
    if(D3DGraphics::Active()) return D3DGraphics::LineWidth(value);
    return ::glLineWidth(value);
}
inline void APIENTRY glPolygonMode(GLenum argument0,GLenum argument1) {
    if(D3DGraphics::Active()) return D3DGraphics::PolygonMode(argument0,argument1);
    return ::glPolygonMode(argument0,argument1);
}
inline void APIENTRY glViewport(GLint x,GLint y,GLsizei width,GLsizei height) {
    if(D3DGraphics::Active()) return D3DGraphics::Viewport(x,y,width,height);
    return ::glViewport(x,y,width,height);
}
inline void APIENTRY glPushAttrib(GLbitfield argument0) {
    if(D3DGraphics::Active()) return D3DGraphics::PushAttrib(argument0);
    return ::glPushAttrib(argument0);
}
inline void APIENTRY glPopAttrib() {
    if(D3DGraphics::Active()) return D3DGraphics::PopAttrib();
    return ::glPopAttrib();
}
inline void APIENTRY glPushClientAttrib(GLbitfield argument0) {
    if(D3DGraphics::Active()) return D3DGraphics::PushClientAttrib(argument0);
    return ::glPushClientAttrib(argument0);
}
inline void APIENTRY glPopClientAttrib() {
    if(D3DGraphics::Active()) return D3DGraphics::PopClientAttrib();
    return ::glPopClientAttrib();
}
inline void APIENTRY glPixelStorei(GLenum argument0,GLint argument1) {
    if(D3DGraphics::Active()) return D3DGraphics::PixelStorei(argument0,argument1);
    return ::glPixelStorei(argument0,argument1);
}
inline void APIENTRY glPixelTransferi(GLenum argument0,GLint argument1) {
    if(D3DGraphics::Active()) return D3DGraphics::PixelTransferi(argument0,argument1);
    return ::glPixelTransferi(argument0,argument1);
}
inline void APIENTRY glPixelTransferf(GLenum argument0,GLfloat argument1) {
    if(D3DGraphics::Active()) return D3DGraphics::PixelTransferf(argument0,argument1);
    return ::glPixelTransferf(argument0,argument1);
}
inline void APIENTRY glTexEnvi(GLenum argument0,GLenum argument1,GLint argument2) {
    if(D3DGraphics::Active()) return D3DGraphics::TexEnvi(argument0,argument1,argument2);
    return ::glTexEnvi(argument0,argument1,argument2);
}
inline void APIENTRY glMatrixMode(GLenum mode) {
    if(D3DGraphics::Active()) return D3DGraphics::MatrixMode(mode);
    return ::glMatrixMode(mode);
}
inline void APIENTRY glLoadIdentity() {
    if(D3DGraphics::Active()) return D3DGraphics::LoadIdentity();
    return ::glLoadIdentity();
}
inline void APIENTRY glLoadMatrixf(const GLfloat* value) {
    if(D3DGraphics::Active()) return D3DGraphics::LoadMatrixf(value);
    return ::glLoadMatrixf(value);
}
inline void APIENTRY glPushMatrix() {
    if(D3DGraphics::Active()) return D3DGraphics::PushMatrix();
    return ::glPushMatrix();
}
inline void APIENTRY glPopMatrix() {
    if(D3DGraphics::Active()) return D3DGraphics::PopMatrix();
    return ::glPopMatrix();
}
inline void APIENTRY glTranslatef(GLfloat x,GLfloat y,GLfloat z) {
    if(D3DGraphics::Active()) return D3DGraphics::Translatef(x,y,z);
    return ::glTranslatef(x,y,z);
}
inline void APIENTRY glScalef(GLfloat x,GLfloat y,GLfloat z) {
    if(D3DGraphics::Active()) return D3DGraphics::Scalef(x,y,z);
    return ::glScalef(x,y,z);
}
inline void APIENTRY glRotatef(GLfloat angle,GLfloat x,GLfloat y,GLfloat z) {
    if(D3DGraphics::Active()) return D3DGraphics::Rotatef(angle,x,y,z);
    return ::glRotatef(angle,x,y,z);
}
inline void APIENTRY glOrtho(GLdouble l,GLdouble r,GLdouble b,GLdouble t,GLdouble n,GLdouble f) {
    if(D3DGraphics::Active()) return D3DGraphics::Ortho(l,r,b,t,n,f);
    return ::glOrtho(l,r,b,t,n,f);
}
inline void APIENTRY glFrustum(GLdouble l,GLdouble r,GLdouble b,GLdouble t,GLdouble n,GLdouble f) {
    if(D3DGraphics::Active()) return D3DGraphics::Frustum(l,r,b,t,n,f);
    return ::glFrustum(l,r,b,t,n,f);
}
inline void APIENTRY glGetIntegerv(GLenum name,GLint* result) {
    if(D3DGraphics::Active()) return D3DGraphics::GetIntegerv(name,result);
    return ::glGetIntegerv(name,result);
}
inline void APIENTRY glGetFloatv(GLenum name,GLfloat* result) {
    if(D3DGraphics::Active()) return D3DGraphics::GetFloatv(name,result);
    return ::glGetFloatv(name,result);
}
inline void APIENTRY glGetBooleanv(GLenum name,GLboolean* result) {
    if(D3DGraphics::Active()) return D3DGraphics::GetBooleanv(name,result);
    return ::glGetBooleanv(name,result);
}
inline void APIENTRY glGetTexEnviv(GLenum argument0,GLenum argument1,GLint* value) {
    if(D3DGraphics::Active()) return D3DGraphics::GetTexEnviv(argument0,argument1,value);
    return ::glGetTexEnviv(argument0,argument1,value);
}
inline GLenum APIENTRY glGetError() {
    if(D3DGraphics::Active()) return D3DGraphics::GetError();
    return ::glGetError();
}
inline const GLubyte* APIENTRY glGetString(GLenum name) {
    if(D3DGraphics::Active()) return D3DGraphics::GetString(name);
    return ::glGetString(name);
}
inline HGLRC WINAPI wglGetCurrentContext() {
    if(D3DGraphics::Active()) return D3DGraphics::GetCurrentContext();
    return ::wglGetCurrentContext();
}
inline HDC WINAPI wglGetCurrentDC() {
    if(D3DGraphics::Active()) return D3DGraphics::GetCurrentDC();
    return ::wglGetCurrentDC();
}
inline PROC WINAPI wglGetProcAddress(LPCSTR argument0) {
    if(D3DGraphics::Active()) return D3DGraphics::GetProcAddress(argument0);
    return ::wglGetProcAddress(argument0);
}
inline void APIENTRY glGenTextures(GLsizei count,GLuint* names) {
    if(D3DGraphics::Active()) return D3DGraphics::GenTextures(count,names);
    return ::glGenTextures(count,names);
}
inline void APIENTRY glBindTexture(GLenum target,GLuint name) {
    if(D3DGraphics::Active()) return D3DGraphics::BindTexture(target,name);
    return ::glBindTexture(target,name);
}
inline void APIENTRY glDeleteTextures(GLsizei count,const GLuint* names) {
    if(D3DGraphics::Active()) return D3DGraphics::DeleteTextures(count,names);
    return ::glDeleteTextures(count,names);
}
inline GLboolean APIENTRY glIsTexture(GLuint name) {
    if(D3DGraphics::Active()) return D3DGraphics::IsTexture(name);
    return ::glIsTexture(name);
}
inline void APIENTRY glTexParameteri(GLenum target,GLenum name,GLint value) {
    if(D3DGraphics::Active()) return D3DGraphics::TexParameteri(target,name,value);
    return ::glTexParameteri(target,name,value);
}
inline void APIENTRY glTexImage2D(GLenum target,GLint level,GLint internal,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels) {
    if(D3DGraphics::Active()) return D3DGraphics::TexImage2D(target,level,internal,width,height,border,format,type,pixels);
    return ::glTexImage2D(target,level,internal,width,height,border,format,type,pixels);
}
inline void APIENTRY glGetTexLevelParameteriv(GLenum target,GLint argument1,GLenum name,GLint* value) {
    if(D3DGraphics::Active()) return D3DGraphics::GetTexLevelParameteriv(target,argument1,name,value);
    return ::glGetTexLevelParameteriv(target,argument1,name,value);
}
}
#define glBegin GraphicsApi::glBegin
#define glEnd GraphicsApi::glEnd
#define glVertex3f GraphicsApi::glVertex3f
#define glVertex2f GraphicsApi::glVertex2f
#define glVertex2i GraphicsApi::glVertex2i
#define glTexCoord2f GraphicsApi::glTexCoord2f
#define glTexCoord3f GraphicsApi::glTexCoord3f
#define glColor4f GraphicsApi::glColor4f
#define glColor3f GraphicsApi::glColor3f
#define glEnable GraphicsApi::glEnable
#define glDisable GraphicsApi::glDisable
#define glIsEnabled GraphicsApi::glIsEnabled
#define glDepthMask GraphicsApi::glDepthMask
#define glDepthFunc GraphicsApi::glDepthFunc
#define glDepthRange GraphicsApi::glDepthRange
#define glClearDepth GraphicsApi::glClearDepth
#define glClear GraphicsApi::glClear
#define glAlphaFunc GraphicsApi::glAlphaFunc
#define glBlendFunc GraphicsApi::glBlendFunc
#define glColorMask GraphicsApi::glColorMask
#define glLineWidth GraphicsApi::glLineWidth
#define glPolygonMode GraphicsApi::glPolygonMode
#define glViewport GraphicsApi::glViewport
#define glPushAttrib GraphicsApi::glPushAttrib
#define glPopAttrib GraphicsApi::glPopAttrib
#define glPushClientAttrib GraphicsApi::glPushClientAttrib
#define glPopClientAttrib GraphicsApi::glPopClientAttrib
#define glPixelStorei GraphicsApi::glPixelStorei
#define glPixelTransferi GraphicsApi::glPixelTransferi
#define glPixelTransferf GraphicsApi::glPixelTransferf
#define glTexEnvi GraphicsApi::glTexEnvi
#define glMatrixMode GraphicsApi::glMatrixMode
#define glLoadIdentity GraphicsApi::glLoadIdentity
#define glLoadMatrixf GraphicsApi::glLoadMatrixf
#define glPushMatrix GraphicsApi::glPushMatrix
#define glPopMatrix GraphicsApi::glPopMatrix
#define glTranslatef GraphicsApi::glTranslatef
#define glScalef GraphicsApi::glScalef
#define glRotatef GraphicsApi::glRotatef
#define glOrtho GraphicsApi::glOrtho
#define glFrustum GraphicsApi::glFrustum
#define glGetIntegerv GraphicsApi::glGetIntegerv
#define glGetFloatv GraphicsApi::glGetFloatv
#define glGetBooleanv GraphicsApi::glGetBooleanv
#define glGetTexEnviv GraphicsApi::glGetTexEnviv
#define glGetError GraphicsApi::glGetError
#define glGetString GraphicsApi::glGetString
#define wglGetCurrentContext GraphicsApi::wglGetCurrentContext
#define wglGetCurrentDC GraphicsApi::wglGetCurrentDC
#define wglGetProcAddress GraphicsApi::wglGetProcAddress
#define glGenTextures GraphicsApi::glGenTextures
#define glBindTexture GraphicsApi::glBindTexture
#define glDeleteTextures GraphicsApi::glDeleteTextures
#define glIsTexture GraphicsApi::glIsTexture
#define glTexParameteri GraphicsApi::glTexParameteri
#define glTexImage2D GraphicsApi::glTexImage2D
#define glGetTexLevelParameteriv GraphicsApi::glGetTexLevelParameteriv
#endif

namespace GraphicsBackend {
inline bool IsD3D9() {
#ifdef WCS_D3D9
    return D3DGraphics::Active();
#else
    return false;
#endif
}
}
