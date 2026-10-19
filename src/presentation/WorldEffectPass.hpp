#pragma once
#include "WorldLabelView.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <cmath>

// Shared world-space projection/depth scope for private projectile meshes and their effects.
class WorldEffectPass {
public:
    WorldEffectPass(const WorldLabelView& view,float nearPlane,float farPlane) {
        glGetIntegerv(GL_MATRIX_MODE,&mode_);glPushAttrib(GL_ALL_ATTRIB_BITS);
        glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();float top=std::tan(view.verticalFov*.01745329252f*.5f)*nearPlane;
        glFrustum(-top*view.aspect,top*view.aspect,-top,top,nearPlane,farPlane);
        float cy=std::cos(view.yaw*.01745329252f),sy=std::sin(view.yaw*.01745329252f),cp=std::cos(view.pitch*.01745329252f),sp=std::sin(view.pitch*.01745329252f);
        float matrix[]={sy,-cy*sp,-cy*cp,0,-cy,-sy*sp,-sy*cp,0,0,cp,-sp,0,
            -sy*view.eye[0]+cy*view.eye[1],cy*sp*view.eye[0]+sy*sp*view.eye[1]-cp*view.eye[2],cy*cp*view.eye[0]+sy*cp*view.eye[1]+sp*view.eye[2],1};
        glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadMatrixf(matrix);
        glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);
        glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(1,1,1,1);
        for(int i=0;i<6;++i)glDisable(GL_CLIP_PLANE0+i);
        glEnable(GL_ALPHA_TEST);glAlphaFunc(GL_GREATER,.1f);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);glDepthRange(0,1);
        glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    }
    ~WorldEffectPass(){glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();glMatrixMode(mode_);}
    WorldEffectPass(const WorldEffectPass&)=delete;
    WorldEffectPass& operator=(const WorldEffectPass&)=delete;
private:GLint mode_=GL_MODELVIEW;
};
