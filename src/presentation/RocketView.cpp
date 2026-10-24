#include "GraphicsApi.hpp"
#include "RocketView.hpp"
#include "WorldEffectPass.hpp"
#include "../platform/WarcraftApi.hpp"
#include <algorithm>

void RocketView::Reset(bool deleteTextures){model_.Reset(deleteTextures);if(deleteTextures && puff_)glDeleteTextures(1,&puff_);puff_=0;loaded_=attempted_=false;}
void RocketView::Draw(const std::string& root,const std::vector<RocketVisual>& items,bool laser,const GrenadePoint& spot,
    const WorldLabelView& view,int width,int height,float nearPlane,float farPlane) {
    if((items.empty() && !laser) || !view.active || width<=0 || height<=0 || nearPlane<=0 || farPlane<=nearPlane)return;
    WorldEffectPass pass(view,nearPlane,farPlane);
    if(!attempted_ && !items.empty()){attempted_=true;loaded_=model_.Load(root+"/assets/world/rpgrocket.wcg");wc3::Log("RPG world model loaded=%d",loaded_);}
    if(loaded_)for(const auto& item:items)if(!item.explosion) {
        float yaw=std::atan2(item.direction[1],item.direction[0])*57.2957795f,pitch=std::atan2(item.direction[2],std::hypot(item.direction[0],item.direction[1]))*57.2957795f;
        glPushMatrix();glTranslatef(item.position[0],item.position[1],item.position[2]);glRotatef(yaw,0,0,1);glRotatef(-pitch,0,1,0);
        glScalef(item.scale,item.scale,item.scale);model_.DrawWorld();glPopMatrix();
    }
    if(!puff_) {
        // Small original radial coverage supplies soft smoke and glow without distributing any retail sprite.
        unsigned char pixels[32*32*4];for(int y=0;y<32;++y)for(int x=0;x<32;++x){int at=(y*32+x)*4;float r=std::hypot((x-15.5f)/15.5f,(y-15.5f)/15.5f);
            pixels[at]=pixels[at+1]=pixels[at+2]=255;pixels[at+3]=static_cast<unsigned char>(std::clamp(1-r,0.f,1.f)*255);}
        glGenTextures(1,&puff_);glBindTexture(GL_TEXTURE_2D,puff_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);glPopClientAttrib();
    }
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,puff_);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);glAlphaFunc(GL_GREATER,.005f);
    float cy=std::cos(view.yaw*.01745329252f),sy=std::sin(view.yaw*.01745329252f),cp=std::cos(view.pitch*.01745329252f),sp=std::sin(view.pitch*.01745329252f);
    auto puff=[&](const GrenadePoint& point,float size,float r,float g,float b,float alpha) {
        glColor4f(r,g,b,alpha);glBegin(GL_QUADS);
        for(int i=0;i<4;++i){float x=i==0 || i==3 ? -size : size,y=i<2 ? -size : size;
            glTexCoord2f(x>0 ? 1.f : 0.f,y>0 ? 1.f : 0.f);glVertex3f(point[0]+sy*x-cy*sp*y,point[1]-cy*x-sy*sp*y,point[2]+cp*y);}
        glEnd();
    };
    for(const auto& item:items) {
        if(item.explosion){float age=std::clamp(item.age/.65f,0.f,1.f);puff(item.position,item.scale*(.25f+age),1,.5f*(1-age),.08f,(1-age)*.95f);continue;}
        if(item.trailSize>0)for(size_t i=0;i<item.trail.size();++i){float old=1-float(i)/std::max(size_t(1),item.trail.size());puff(item.trail[i],item.trailSize*(1+old),.8f,.8f,.85f,(1-old)*.35f);}
        if(item.powered)puff(item.position,8,1,.65f,.2f,.9f);
    }
    if(laser)puff(spot,3,1,.02f,.01f,1);
}
