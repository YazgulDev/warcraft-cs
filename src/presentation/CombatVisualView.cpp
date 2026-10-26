#include "CombatVisualView.hpp"
#include "WorldEffectPass.hpp"
#include "HealthBarView.hpp"

namespace {
void Drop(float radius,const WorldLabelView& view,float alpha) {
    // Soft camera-facing droplets read as a blood spray rather than solid crystal shards; no retail sprite is needed.
    float yaw=view.yaw*.01745329252f,pitch=view.pitch*.01745329252f;
    float right[]={std::sin(yaw),-std::cos(yaw),0},up[]={-std::cos(yaw)*std::sin(pitch),-std::sin(yaw)*std::sin(pitch),std::cos(pitch)};
    glBegin(GL_TRIANGLE_FAN);glColor4f(.7f,.008f,.004f,alpha);glVertex3f(0,0,0);
    for(int i=0;i<=10;++i){float a=i*6.2831853f/10,x=std::cos(a)*radius,y=std::sin(a)*radius*1.5f;
        glColor4f(.45f,0,0,0);glVertex3f(right[0]*x+up[0]*y,right[1]*x+up[1]*y,right[2]*x+up[2]*y);}glEnd();
}
void Case(float size,bool shotgun) {
    // Six brass faces, a dark open mouth and a primer; shotgun hulls retain their red plastic sleeve.
    float radius=size*.8f,length=size*3.5f;
    for(int i=0;i<6;++i){float a=i*6.2831853f/6,b=(i+1)*6.2831853f/6;
        if(shotgun)glColor4f(.65f,.045f,.025f,1);else glColor4f(i%2 ? .95f : .6f,i%2 ? .67f : .38f,.12f,1);
        glBegin(GL_QUADS);glVertex3f(std::cos(a)*radius,std::sin(a)*radius,-length);glVertex3f(std::cos(b)*radius,std::sin(b)*radius,-length);
        glVertex3f(std::cos(b)*radius,std::sin(b)*radius,length);glVertex3f(std::cos(a)*radius,std::sin(a)*radius,length);glEnd();
    }
    for(int side:{-1,1}){if(side<0)glColor3f(.8f,.5f,.12f);else glColor3f(.16f,.10f,.035f);
        glBegin(GL_TRIANGLE_FAN);glVertex3f(0,0,side*length);
        for(int i=0;i<=6;++i){float a=i*6.2831853f/6;glVertex3f(std::cos(a)*radius,std::sin(a)*radius,side*length);}glEnd();}
}
}
void CombatVisualView::Draw(const CombatVisuals& effects,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane) {
    if(!view.active || width<=0 || height<=0 || nearPlane<=0 || farPlane<=nearPlane)return;
    {
        // Run before the held weapon clears world depth. Particles/cases never write into native scenery depth.
        WorldEffectPass pass(view,nearPlane,farPlane);glDisable(GL_TEXTURE_2D);glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        for(const auto& p:effects.BloodDrops()){
            glPushMatrix();glTranslatef(p.position[0],p.position[1],p.position[2]);
            Drop(p.size,view,1-p.age/p.lifetime);glPopMatrix();
        }
        for(const auto& p:effects.Shells()){
            glPushMatrix();glTranslatef(p.position[0],p.position[1],p.position[2]);if(p.resting){glRotatef(p.angle,0,0,1);glRotatef(90,1,0,0);}else glRotatef(p.angle,1,.7f,.3f);
            Case(p.size,p.shotgun);glPopMatrix();
        }
    }
    // The requested Headshot caption remains above its accepted world contact, never centered on the reticle.
    auto projection=view;projection.maximumDistance=farPlane;
    for(const auto& c:effects.Criticals()){
        UnitHealthBar anchor={{c.position[0],c.position[1],c.position[2]+c.age*c.rise},1,1,false};HealthBarPlacement at;
        if(HealthBarView::Project(anchor,projection,float(width),float(height),nearPlane,farPlane,at))
            caption_.DrawAt("Headshot",width,height,int(std::clamp(height/45,16,30)*c.scale),at.x,at.y,
                1-c.age/c.lifetime,at.depth);
    }
}
