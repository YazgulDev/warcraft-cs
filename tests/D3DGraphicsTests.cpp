#include "../src/presentation/GraphicsApi.hpp"
#include "../src/presentation/HudFont.hpp"
#include "../src/presentation/RenderMatrix.hpp"
#include "../src/presentation/EnergyBeamView.hpp"
#include "../src/platform/DiagnosticLog.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iterator>

namespace {
void Require(bool condition,const char* message) {if(!condition){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
void Rect(float x,float y,float width,float height,float z=0) {
    glBegin(GL_QUADS);glVertex3f(x,y,z);glVertex3f(x+width,y,z);glVertex3f(x+width,y+height,z);glVertex3f(x,y+height,z);glEnd();
}
}
int main(int argc,char** argv) {
    Require(argc==2,"private diagnostic journal directory");
    LoggingSettings logging;logging.detailed=false;DiagnosticLog::Configure(logging);DiagnosticLog::Open(argv[1]);
    auto nearProjection=RenderMatrix::Identity();nearProjection[10]=2;nearProjection[14]=-1;
    auto d3dProjection=RenderMatrix::DepthForD3D(nearProjection);
    Require(d3dProjection[10]==1 && d3dProjection[14]==0,"clip-depth conversion preserves homogeneous W");
    HWND window=CreateWindowExA(0,"STATIC","WarcraftCS D3D test",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);
    Require(window!=nullptr,"hidden test window");
    auto api=Direct3DCreate9(D3D_SDK_VERSION);Require(api!=nullptr,"D3D9 runtime");
    D3DPRESENT_PARAMETERS params={};params.Windowed=TRUE;params.SwapEffect=D3DSWAPEFFECT_DISCARD;params.hDeviceWindow=window;
    params.BackBufferWidth=320;params.BackBufferHeight=240;params.BackBufferFormat=D3DFMT_A8R8G8B8;
    params.EnableAutoDepthStencil=TRUE;params.AutoDepthStencilFormat=D3DFMT_D24S8;
    IDirect3DDevice9* device=nullptr;
    Require(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&params,&device)),"hidden HAL device");
    device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFF101010,1,0);
    // A renderer must isolate hostile game state and restore it after its last draw batch.
    device->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_RED);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_CW);
    HDC dc=GetDC(window);Require(!GraphicsBackend::IsD3D9(),"legacy forwarding outside D3D frame");
    // Force a real rejected D3D call; the adapter must log it and retain its existing failure/recovery contract.
    Require(SUCCEEDED(device->BeginScene()),"host-owned scene");
    Require(!D3DGraphics::BeginFrame(device,dc,320,240),"nested BeginScene rejected");
    Require(SUCCEEDED(device->EndScene()),"host scene remains owned by caller");
    Require(D3DGraphics::BeginFrame(device,dc,320,240),"begin isolated frame");
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,320,240,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();glDisable(GL_DEPTH_TEST);
    glColor4f(1,0,0,1);Rect(10,10,50,50);
    glPushAttrib(GL_ALL_ATTRIB_BITS);glColor4f(0,1,0,1);Rect(70,10,50,50);glPopAttrib();Rect(130,10,50,50);
    // Preserve world occlusion before the held weapon's independent depth clear.
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);
    glColor4f(1,0,0,1);Rect(10,80,50,40,.6f);glColor4f(0,1,0,1);Rect(10,80,50,40,.3f);glDisable(GL_DEPTH_TEST);
    HudFont font("d3d-regression");Require(font.Ensure(22),"GDI alpha atlas uploads through D3D");
    glColor4f(1,1,1,1);font.Draw(12,160,"WARCRAFT 1.27a");
    GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    const unsigned char blue[]={0,0,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,blue);
    glEnable(GL_TEXTURE_2D);glColor4f(1,1,1,1);glTexCoord2f(0,0);Rect(200,10,40,40);glDisable(GL_TEXTURE_2D);
    D3DGraphics::EndFrame();Require(!GraphicsBackend::IsD3D9(),"end frame releases dispatch ownership");
    DWORD state=0;device->GetRenderState(D3DRS_COLORWRITEENABLE,&state);Require(state==D3DCOLORWRITEENABLE_RED,"host color mask restored");
    device->GetRenderState(D3DRS_CULLMODE,&state);Require(state==D3DCULL_CW,"host raster state restored");
    IDirect3DSurface9 *back=nullptr,*copy=nullptr;Require(SUCCEEDED(device->GetRenderTarget(0,&back)),"back buffer");
    Require(SUCCEEDED(device->CreateOffscreenPlainSurface(320,240,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&copy,nullptr)),"readback surface");
    Require(SUCCEEDED(device->GetRenderTargetData(back,copy)),"actual D3D pixel readback");
    D3DLOCKED_RECT pixels={};Require(SUCCEEDED(copy->LockRect(&pixels,nullptr,D3DLOCK_READONLY)),"readback lock");
    auto pixel=[&](int x,int y){return reinterpret_cast<const DWORD*>(static_cast<const unsigned char*>(pixels.pBits)+size_t(y)*pixels.Pitch)[x]&0xFFFFFF;};
    std::printf("D3D pixels red=%06lX green=%06lX restored=%06lX depth=%06lX blue=%06lX\n",pixel(25,25),pixel(85,25),pixel(145,25),pixel(25,95),pixel(215,25));
    Require(pixel(25,25)==0xFF0000 && pixel(85,25)==0x00FF00 && pixel(145,25)==0xFF0000,"quad triangulation, color and attribute isolation");
    Require(pixel(25,95)==0xFF0000,"world-depth occlusion agrees with converted clip coordinates");
    Require(pixel(215,25)==0x0000FF,"RGBA textures retain channel order");
    int ink=0;for(int y=135;y<165;++y)for(int x=10;x<230;++x)if(pixel(x,y)!=0x101010)++ink;
    Require(ink>100 && ink<3000,"font coverage has readable glyphs and transparent spaces");
    copy->UnlockRect();copy->Release();back->Release();
    Require(D3DGraphics::BeginFrame(device,dc,320,240),"second frame reuses device resources");
    Require(glIsTexture(texture)==GL_TRUE && font.Valid(),"texture generation survives presentations");
    glDeleteTextures(1,&texture);font.Reset(true);D3DGraphics::EndFrame();
    // Newly merged Tau/Gluon views must submit real D3D pixels and retain native-world depth occlusion.
    for(bool gluon:{false,true})for(float depth:{1.f,0.f}) {
        device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFF000000,depth,0);
        Require(D3DGraphics::BeginFrame(device,dc,320,240),"energy beam frame");
        WorldLabelView view;view.active=true;view.eye[2]=96;view.aspect=320.f/240;
        EnergyBeam beam;beam.start={50,-30,96};beam.end={100,30,96};beam.tick=GetTickCount();beam.seconds=1;beam.width=3;beam.gluon=gluon;
        beam.continuous=gluon;beam.impact=true; // Exercise animated strands and the contact billboard as well as the core.
        GLuint effectTexture=0;glGenTextures(1,&effectTexture);glBindTexture(GL_TEXTURE_2D,effectTexture);
        const unsigned char white[4]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);
        EnergyBeamView::Draw(beam,view,8,5000,effectTexture,effectTexture);glDeleteTextures(1,&effectTexture);D3DGraphics::EndFrame();
        IDirect3DSurface9 *target=nullptr,*readback=nullptr;
        Require(SUCCEEDED(device->GetRenderTarget(0,&target)),"beam target");
        Require(SUCCEEDED(device->CreateOffscreenPlainSurface(320,240,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr)),"beam readback");
        Require(SUCCEEDED(device->GetRenderTargetData(target,readback)),"beam pixels");
        D3DLOCKED_RECT data={};Require(SUCCEEDED(readback->LockRect(&data,nullptr,D3DLOCK_READONLY)),"beam pixel lock");
        int ink=0;for(int y=0;y<240;++y)for(int x=0;x<320;++x)
            ink+=(reinterpret_cast<const DWORD*>(static_cast<const unsigned char*>(data.pBits)+size_t(y)*data.Pitch)[x]&0xFFFFFF)!=0;
        Require(depth==0 ? ink==0 : ink>50,"Tau/Gluon D3D pixels and world occlusion");
        readback->UnlockRect();readback->Release();target->Release();
    }
    ReleaseDC(window,dc);device->Release();api->Release();DestroyWindow(window);
    DiagnosticLog::Close();std::ifstream journal(std::string(argv[1])+"/WarcraftCS.log",std::ios::binary);
    std::string text{std::istreambuf_iterator<char>(journal),std::istreambuf_iterator<char>()};
    Require(text.find("graphics D3D9 operation=BeginScene hr=8876086C")!=std::string::npos,"actual rejected call logged with HRESULT while TRACE disabled");
    std::printf("PASS D3D9 actual pixels, atlas coverage, depth, resource lifetime, state restoration and real failure diagnostics.\n");
}
