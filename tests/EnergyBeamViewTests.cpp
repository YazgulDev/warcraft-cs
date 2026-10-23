#include "../src/presentation/EnergyBeamView.hpp"
#include "../src/presentation/ViewmodelAnchor.hpp"
#include "../src/presentation/EnergySpriteCache.hpp"
#include <filesystem>
#include <cassert>
#include <vector>
#include <cstdio>

int main() {
    // An authored held-model muzzle must project to the identical screen point under native FOV,
    // arbitrary camera heading/pitch, custom model framing and the decorative movement pose.
    for(float fov:{25.f,50.f,85.f})for(float yaw:{0.f,90.f,270.f})for(float pitch:{-50.f,0.f,50.f})for(float scale:{.25f,1.f,2.f})for(float heldAspect:{1.33f,2.05f})for(float nearPlane:{8.f,100.f}) {
        WorldLabelView view;view.eye[0]=120;view.eye[1]=-500;view.eye[2]=96;view.verticalFov=fov;view.yaw=yaw;view.pitch=pitch;view.aspect=2.05f;
        ViewmodelPose pose={1.2f,-.5f,.2f,10};std::array<float,3> anchor={-5.27f,-8.88f,-20.08f};
        auto point=ViewmodelAnchor::World(anchor,pose,scale,view,heldAspect,nearPlane);float delta[3];for(int i=0;i<3;++i)delta[i]=point[i]-view.eye[i];
        constexpr float radians=.01745329252f;float cy=std::cos(yaw*radians),sy=std::sin(yaw*radians),cp=std::cos(pitch*radians),sp=std::sin(pitch*radians);
        float depth=delta[0]*cy*cp+delta[1]*sy*cp+delta[2]*sp;
        float right=delta[0]*sy-delta[1]*cy,up=-delta[0]*cy*sp-delta[1]*sy*sp+delta[2]*cp;
        float x=anchor[0]*std::cos(pose.roll*radians)-anchor[1]*std::sin(pose.roll*radians)+pose.x;
        float y=anchor[0]*std::sin(pose.roll*radians)+anchor[1]*std::cos(pose.roll*radians)+pose.y;
        float z=anchor[2]+pose.z,tan=std::tan(fov*radians*.5f);
        assert(depth>nearPlane); // The visible start survives even a near plane beyond the held muzzle.
        assert(std::abs(right/(depth*tan*view.aspect)-x/(-z*scale*heldAspect))<.00002f);
        assert(std::abs(up/(depth*tan)-y/(-z*scale))<.00002f);
    }
    HWND window=CreateWindowA("STATIC","Energy beam",WS_POPUP,0,0,256,256,nullptr,nullptr,nullptr,nullptr);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;
    format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;format.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));
    for(int cycle=0;cycle<2;++cycle) {
        HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));glViewport(0,0,256,256);
        WorldLabelView view;view.active=true;view.eye[2]=96;
        EnergyBeam beam;beam.start={50,-30,96};beam.end={100,30,96};beam.tick=GetTickCount();beam.seconds=1;beam.width=3;
        auto clear=[](float depth) {glColorMask(1,1,1,1);glDepthMask(1);glClearDepth(depth);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);};
        auto ink=[] {std::vector<unsigned char> pixels(256*256*4);glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());int result=0;
            for(size_t i=0;i<pixels.size();i+=4)if(pixels[i]>20 || pixels[i+1]>20 || pixels[i+2]>20)++result;return result;};
        for(bool gluon:{false,true}) {
            beam.gluon=gluon;clear(1);EnergyBeamView::Draw(beam,view,8,5000);assert(ink()>50);
            clear(0);EnergyBeamView::Draw(beam,view,8,5000);assert(ink()==0); // Native geometry occludes both beam colors.
        }
        // Animated impact art is context-owned and validated before upload; both textured passes respect native depth.
        auto artPath=std::filesystem::temp_directory_path()/("wcs-energy-"+std::to_string(GetCurrentProcessId())+".wes");
        {std::ofstream art(artPath,std::ios::binary);art.write("WES1",4);uint32_t count=2,w=2,h=2;art.write(reinterpret_cast<char*>(&count),4);
            for(unsigned f=0;f<count;++f){art.write(reinterpret_cast<char*>(&w),4);art.write(reinterpret_cast<char*>(&h),4);unsigned char pixels[16];std::fill_n(pixels,16,255);art.write(reinterpret_cast<char*>(pixels),16);}}
        EnergySpriteCache sprites;auto frame=sprites.Frame(artPath.string(),0);assert(frame && sprites.Valid() && sprites.Frame(artPath.string(),1)!=frame);
        beam.gluon=beam.continuous=beam.impact=true;
        clear(1);EnergyBeamView::Draw(beam,view,8,5000,frame,frame);assert(ink()>50);
        clear(0);EnergyBeamView::Draw(beam,view,8,5000,frame,frame);assert(ink()==0);
        sprites.Reset(true);assert(!glIsTexture(frame));
        {std::ofstream corrupt(artPath,std::ios::binary);corrupt.write("WES1",4);}
        assert(!sprites.Frame(artPath.string(),0));sprites.Reset(true);std::filesystem::remove(artPath);
        beam.continuous=beam.impact=false;
        // A wide held viewport and a distant native near plane must still light the nozzle pixel, not leave a gap.
        clear(1);view.aspect=1.33f;beam.start=ViewmodelAnchor::World({-5,-8,-20},{},1,view,2.05f,100);beam.end={1000,0,96};
        EnergyBeamView::Draw(beam,view,100,5000);
        int px=int((1-5.f/(20*2.05f))*128); // Held horizontal projection, independent of native aspect.
        int py=int((1-8.f/20)*128);unsigned char nozzle[4*4*4]={};glReadPixels(px-1,py-1,4,4,GL_RGBA,GL_UNSIGNED_BYTE,nozzle);
        bool joined=false;for(size_t i=0;i<sizeof(nozzle);i+=4)joined|=nozzle[i]>20 || nozzle[i+1]>20 || nozzle[i+2]>20;assert(joined);
        view.aspect=1;beam.start={50,-30,96};beam.end={100,30,96};
        clear(1);glColorMask(0,0,0,0);glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glEnable(GL_COLOR_LOGIC_OP);glLogicOp(GL_NOOP);
        EnergyBeamView::Draw(beam,view,8,5000);assert(ink()>50);
        GLboolean mask[4]={};glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && glIsEnabled(GL_COLOR_LOGIC_OP));
        GLint polygon[2]={};glGetIntegerv(GL_POLYGON_MODE,polygon);assert(polygon[0]==GL_LINE);
        glDisable(GL_COLOR_LOGIC_OP);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);clear(1);beam.tick=GetTickCount()-2000;
        EnergyBeamView::Draw(beam,view,8,5000);assert(ink()==0 && glGetError()==GL_NO_ERROR);
        wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    ReleaseDC(window,dc);DestroyWindow(window);
    std::puts("PASS real WGL Tau/Gluon beam pixels, occlusion, expiry, inherited state restoration and context replacement.");
}
