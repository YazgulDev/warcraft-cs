#pragma once
#include "EnergyBeam.hpp"
#include "GraphicsApi.hpp" // Use the same OpenGL/D3D9 dispatch as held models and other world effects.
#include "WorldEffectPass.hpp"
#include "HudRenderState.hpp"
#include <algorithm>

namespace EnergyBeamView {
inline void Draw(const EnergyBeam& beam,const WorldLabelView& view,float nearPlane,float farPlane,GLuint beamTexture=0,GLuint impactTexture=0) {
    float age=(GetTickCount()-beam.tick)*.001f;
    if(!beam.tick || age>beam.seconds || !view.active || nearPlane<=0 || farPlane<=nearPlane)return;
    HudRenderState raster; // Reuse complete blend/coverage/texture isolation, then restore world depth/projection.
    WorldEffectPass pass(view,nearPlane,farPlane);
    // Native scene depth occludes the additive beam and impact; neither effect changes world depth.
    glDisable(GL_TEXTURE_2D);glDisable(GL_ALPHA_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
    float direction[3],towards[3],side[3];
    for(int i=0;i<3;++i){direction[i]=beam.end[i]-beam.start[i];towards[i]=(beam.end[i]+beam.start[i])*.5f-view.eye[i];}
    side[0]=direction[1]*towards[2]-direction[2]*towards[1];
    side[1]=direction[2]*towards[0]-direction[0]*towards[2];side[2]=direction[0]*towards[1]-direction[1]*towards[0];
    float length=std::sqrt(side[0]*side[0]+side[1]*side[1]+side[2]*side[2]);
    if(length<.001f){float yaw=view.yaw*.01745329252f;side[0]=std::sin(yaw);side[1]=-std::cos(yaw);side[2]=0;length=1;}
    for(auto& value:side)value/=length;
    float rayLength=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2]);
    if(rayLength<.001f)return;
    float up[3]={side[1]*direction[2]-side[2]*direction[1],side[2]*direction[0]-side[0]*direction[2],side[0]*direction[1]-side[1]*direction[0]};
    for(auto& value:up)value/=rayLength;
    float phase=GetTickCount()*.001f,alpha=beam.continuous ? 1.f : 1-age/beam.seconds;
    int layers=beam.effects && beam.gluon ? 4 : 2;
    for(int layer=0;layer<layers;++layer) {
        bool core=layer==1,strand=layer>=2;
        float width=beam.width*(core ? .24f : strand ? .35f : 1.f);
        glColor4f(core ? 1.f : beam.gluon ? .25f : 1.f,core ? 1.f : beam.gluon ? .38f : .55f,core || beam.gluon ? 1.f : .1f,alpha*(strand ? .6f : .9f));
        if(beamTexture && beam.gluon && !core){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,beamTexture);}else glDisable(GL_TEXTURE_2D);
        // Counter-rotating strands animate continuously; their endpoints stay attached to muzzle and contact.
        const int segments=beam.effects ? 32 : 1;
        glBegin(GL_QUADS);
        for(int segment=0;segment<segments;++segment)for(int corner=0;corner<4;++corner) {
            float t=float(segment+(corner>=2))/segments;
            float angle=t*24+phase*(layer==2 ? 17.f : -21.f),amplitude=strand ? std::sin(t*3.14159265f)*beam.width*2.5f : 0;
            float sign=corner==0 || corner==3 ? -1.f : 1.f;
            float taper=.15f+.85f*std::min(1.f,t*8);
            glTexCoord2f(sign<0 ? 0.f : 1.f,t*rayLength/128-phase*4);
            glVertex3f(beam.start[0]+direction[0]*t+side[0]*(sign*width*taper+std::sin(angle)*amplitude)+up[0]*std::cos(angle)*amplitude,
                beam.start[1]+direction[1]*t+side[1]*(sign*width*taper+std::sin(angle)*amplitude)+up[1]*std::cos(angle)*amplitude,
                beam.start[2]+direction[2]*t+side[2]*(sign*width*taper+std::sin(angle)*amplitude)+up[2]*std::cos(angle)*amplitude);
        }
        glEnd();
    }
    if(!beam.effects || !beam.impact || beam.impactSize<=0)return;
    if(impactTexture && beam.gluon){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,impactTexture);}else glDisable(GL_TEXTURE_2D);
    float cy=std::cos(view.yaw*.01745329252f),sy=std::sin(view.yaw*.01745329252f),cp=std::cos(view.pitch*.01745329252f),sp=std::sin(view.pitch*.01745329252f);
    float size=beam.impactSize*(.85f+.15f*std::sin(phase*35));
    // Pull the billboard a fraction toward the muzzle to prevent z-fighting with the contacted surface.
    glBegin(GL_TRIANGLES);
    for(int i=0;i<16;++i)for(int vertex=0;vertex<3;++vertex) {
        float angle=(i+(vertex==2))*6.2831853f/16;
        float x=vertex ? std::cos(angle) : 0,y=vertex ? std::sin(angle) : 0;
        glColor4f(beam.gluon ? .65f : 1.f,.65f,beam.gluon ? 1.f : .15f,alpha*(vertex && !impactTexture ? 0.f : 1.f));
        glTexCoord2f((x+1)*.5f,(y+1)*.5f);
        glVertex3f(beam.end[0]-direction[0]/rayLength*.75f+size*(sy*x-cy*sp*y),beam.end[1]-direction[1]/rayLength*.75f+size*(-cy*x-sy*sp*y),beam.end[2]-direction[2]/rayLength*.75f+size*cp*y);
    }
    glEnd();
}
}
