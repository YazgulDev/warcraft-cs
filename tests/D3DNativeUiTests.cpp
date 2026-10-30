#include "../src/platform/D3DNativeUi.hpp"
#include "../src/platform/D3DMinimap.hpp"
#include "../src/presentation/D3DGraphics.hpp"
#include <cstdio>
#include <cstdlib>

namespace wc3 {void Log(const char*,...) {}}
namespace {
void Require(bool condition,const char* message) {if(!condition){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
struct Vertex {float x,y,z,w;DWORD color;};
void Triangle(IDirect3DDevice9* device,float x,float y,DWORD color,float z=0,bool depth=false) {
    Vertex vertices[]={{x,y,z,1,color},{x+36,y,z,1,color},{x,y+36,z,1,color}};
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);device->SetTexture(0,nullptr);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_ZENABLE,depth);
    device->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);device->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESS);
    device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG2);device->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    Require(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,vertices,sizeof(Vertex))),"native triangle");
}
DWORD Pixel(IDirect3DDevice9* device,int x,int y) {
    IDirect3DSurface9 *target=nullptr,*copy=nullptr;device->GetRenderTarget(0,&target);
    Require(SUCCEEDED(device->CreateOffscreenPlainSurface(320,240,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&copy,nullptr)),"readback surface");
    Require(SUCCEEDED(device->GetRenderTargetData(target,copy)),"readback");D3DLOCKED_RECT data={};copy->LockRect(&data,nullptr,D3DLOCK_READONLY);
    DWORD result=reinterpret_cast<DWORD*>(static_cast<unsigned char*>(data.pBits)+y*data.Pitch)[x]&0xFFFFFF;
    copy->UnlockRect();copy->Release();target->Release();return result;
}
}
int main() {
    HWND window=CreateWindowExA(0,"STATIC","native UI device oracle",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);
    auto api=Direct3DCreate9(D3D_SDK_VERSION);Require(api && window,"hidden D3D host");
    D3DPRESENT_PARAMETERS params={};params.Windowed=TRUE;params.SwapEffect=D3DSWAPEFFECT_DISCARD;params.hDeviceWindow=window;
    params.BackBufferWidth=320;params.BackBufferHeight=240;params.BackBufferFormat=D3DFMT_A8R8G8B8;params.EnableAutoDepthStencil=TRUE;params.AutoDepthStencilFormat=D3DFMT_D16;
    IDirect3DDevice9* device=nullptr;Require(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&params,&device)),"real HAL device");
    using Cursor=BOOL (STDMETHODCALLTYPE*)(IDirect3DDevice9*,BOOL);
    auto rawCursor=reinterpret_cast<Cursor>((*reinterpret_cast<void***>(device))[12]);
    IDirect3DSurface9* cursorImage=nullptr;
    Require(SUCCEEDED(device->CreateOffscreenPlainSurface(32,32,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&cursorImage,nullptr)),"cursor image");
    D3DLOCKED_RECT cursorPixels={};cursorImage->LockRect(&cursorPixels,nullptr,0);
    for(int y=0;y<32;++y)for(int x=0;x<32;++x)reinterpret_cast<DWORD*>(static_cast<unsigned char*>(cursorPixels.pBits)+y*cursorPixels.Pitch)[x]=0xFF00FF00;
    cursorImage->UnlockRect();Require(SUCCEEDED(device->SetCursorProperties(0,0,cursorImage)),"real device cursor");cursorImage->Release();
    rawCursor(device,TRUE);D3DNativeUi::Attach(device);D3DNativeUi::Configure(true,1);
    // Bypass our slot only to observe the real device visibility; normal engine requests remain logical.
    device->ShowCursor(TRUE);Require(!rawCursor(device,TRUE),"FPS hides the hardware cursor even after an engine show request");device->ShowCursor(TRUE);
    D3DNativeUi::Configure(true,1,false);Require(rawCursor(device,TRUE),"free-cursor menu restores native requested visibility");
    D3DNativeUi::Configure(true,1);D3DNativeUi::Phase(false);
    device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFF101010,1,0);device->BeginScene();Triangle(device,10,10,0xFF0000FF);
    D3DNativeUi::Phase(true);Triangle(device,70,10,0xFF00FF00);
    // All native submission forms and console clears must be suppressed, including cached driver thunks.
    Require(SUCCEEDED(device->DrawPrimitive(D3DPT_TRIANGLELIST,0,1)),"primitive filtered");
    Require(SUCCEEDED(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,3,0,1)),"indexed filtered");
    Require(SUCCEEDED(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,3,1,nullptr,D3DFMT_INDEX16,nullptr,sizeof(Vertex))),"indexed UP filtered");
    device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFFFF0000,1,0);
    D3DNativeUi::Overlay(true);Triangle(device,130,10,0xFF00FF00);D3DNativeUi::Overlay(false);
    D3DVIEWPORT9 portrait={70,180,40,40,0,1};D3DNativeUi::Viewport(device,portrait,320,240);
    Require(portrait.X==0 && portrait.Y==0,"portrait redirects into private local viewport");device->SetViewport(&portrait);Triangle(device,0,0,0xFFFFFF00,.2f,true);
    D3DMATRIX nativeProjection={};nativeProjection.m[0][0]=1;nativeProjection.m[1][1]=1.6f;nativeProjection.m[2][3]=1;
    D3DNativeUi::PortraitProjection(device,nativeProjection);
    Require(nativeProjection.m[0][0]==nativeProjection.m[1][1],"square portrait framing preserves face proportions without changing vertical FOV");
    // The real engine changes projection after portraits and can omit another SetViewport call.
    D3DNativeUi::Projection(device,true);D3DVIEWPORT9 full={0,0,320,240,0,1},restored={};device->GetViewport(&restored);
    Require(restored.X==0 && restored.Y==0 && restored.Width==320 && restored.Height==240,"projection-only portrait end restores the native viewport");
    Triangle(device,70,60,0xFFFF0000);device->EndScene();
    HDC dc=GetDC(window);D3DNativeUi::Overlay(true);Require(D3DGraphics::BeginFrame(device,dc,320,240),"portrait canvas");
    D3DGraphics::MatrixMode(GL_PROJECTION);D3DGraphics::LoadIdentity();D3DGraphics::Ortho(0,320,240,0,-1,1);
    D3DGraphics::MatrixMode(GL_MODELVIEW);D3DGraphics::LoadIdentity();D3DNativeUi::DrawPortrait(200,60,40);D3DGraphics::EndFrame();D3DNativeUi::Overlay(false);
    Require(Pixel(device,15,15)==0x0000FF,"world pixels survive native console clear");
    Require(Pixel(device,75,15)==0x101010,"native cursor and HP geometry suppressed in the world region");
    Require(Pixel(device,135,15)==0x00FF00,"own overlay geometry remains visible");
    Require(Pixel(device,205,65)==0xFFFF00,"actual native portrait pixels reach the HUD");
    // Successive animation frames must replace the face and restore full-size native UI without
    // an extra engine SetViewport call (the static-pixel check previously masked this transition).
    D3DNativeUi::Phase(false);device->BeginScene();D3DNativeUi::Phase(true);
    portrait={70,180,40,40,0,1};D3DNativeUi::Viewport(device,portrait,320,240);device->SetViewport(&portrait);
    // The engine leaves its world-band scissor active at the first portrait batch. A clipped
    // depth clear retains the earlier face's nearer depth and freezes successive skinned frames.
    RECT worldBand={0,40,320,160};device->SetScissorRect(&worldBand);device->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
    Triangle(device,0,0,0xFF00FFFF,.8f,true);
    D3DNativeUi::Projection(device,true);device->GetViewport(&restored);
    Require(restored.Width==320 && restored.Height==240,"animated portrait restores the previous viewport each frame");device->EndScene();
    D3DNativeUi::Overlay(true);Require(D3DGraphics::BeginFrame(device,dc,320,240),"next portrait animation frame");
    D3DGraphics::MatrixMode(GL_PROJECTION);D3DGraphics::LoadIdentity();D3DGraphics::Ortho(0,320,240,0,-1,1);
    D3DGraphics::MatrixMode(GL_MODELVIEW);D3DGraphics::LoadIdentity();D3DNativeUi::DrawPortrait(200,60,40);D3DGraphics::EndFrame();D3DNativeUi::Overlay(false);
    Require(Pixel(device,205,65)==0x00FFFF,"animated portrait changes rather than retaining its previous frame");
    // Capture a real native minimap draw and a nested portrait without touching world pixels.
    D3DNativeUi::Phase(false);device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFF0000FF,.25f,0);
    D3DMinimap::Configure({0,160,80,240},320,240,true);device->BeginScene();D3DNativeUi::Phase(true);
    Triangle(device,10,170,0xFF00FF00);Triangle(device,160,50,0xFFFF0000);
    portrait={100,180,40,40,0,1};D3DNativeUi::Viewport(device,portrait,320,240);device->SetViewport(&portrait);Triangle(device,0,0,0xFFFFFF00);
    D3DNativeUi::Projection(device,true);device->SetViewport(&full);device->EndScene();D3DMinimap::FinishFrame();
    D3DNativeUi::Overlay(true);Require(D3DGraphics::BeginFrame(device,dc,320,240),"native minimap canvas");
    D3DGraphics::MatrixMode(GL_PROJECTION);D3DGraphics::LoadIdentity();D3DGraphics::Ortho(0,320,240,0,-1,1);
    D3DGraphics::MatrixMode(GL_MODELVIEW);D3DGraphics::LoadIdentity();D3DMinimap::Draw({240,0,80,0});D3DNativeUi::DrawPortrait(200,100,40);
    // A HUD/world label behind the saved world depth must remain occluded after private UI depth clears.
    D3DGraphics::Enable(GL_DEPTH_TEST);D3DGraphics::DepthFunc(GL_LESS);D3DGraphics::DepthMask(GL_FALSE);D3DGraphics::Disable(GL_TEXTURE_2D);D3DGraphics::Color4f(1,0,0,1);
    D3DGraphics::Begin(GL_TRIANGLES);D3DGraphics::Vertex3f(10,170,-.5f);D3DGraphics::Vertex3f(46,170,-.5f);D3DGraphics::Vertex3f(10,206,-.5f);D3DGraphics::End();
    D3DGraphics::EndFrame();D3DNativeUi::Overlay(false);
    Require(Pixel(device,255,15)==0x00FF00,"GPU crop contains native terrain/marker pixels at the HUD map location");
    Require(Pixel(device,315,75)==0,"rectangular map letterboxing remains black");
    Require(Pixel(device,15,175)==0x0000FF && Pixel(device,165,55)==0x0000FF,"minimap and console never overwrite expanded world color");
    Require(Pixel(device,205,105)==0xFFFF00,"portrait survives nested native minimap target ownership");
    // An extra RenderWorld inside the same scene must regain the world target without requiring Present.
    D3DNativeUi::Phase(false);device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF000000,1,0);device->BeginScene();
    D3DNativeUi::Phase(true);Triangle(device,10,170,0xFF00FF00);D3DNativeUi::Phase(false);Triangle(device,10,10,0xFFFF0000);device->EndScene();
    Require(Pixel(device,15,15)==0xFF0000 && Pixel(device,15,175)==0,"second world submission restores its target inside the active scene");
    // Device Reset must succeed after releasing every capture surface, and RTS must resume native drawing.
    D3DNativeUi::BeforeReset(device);D3DNativeUi::Configure(false,0);D3DNativeUi::Phase(false);
    Require(SUCCEEDED(device->Reset(&params)),"capture does not retain default-pool surfaces across Reset");
    device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);device->BeginScene();Triangle(device,10,10,0xFFFF0000);device->EndScene();
    Require(Pixel(device,15,15)==0xFF0000,"native RTS geometry restored");
    ReleaseDC(window,dc);device->Release();api->Release();DestroyWindow(window);
    std::puts("PASS native D3D UI filtering, all draw forms, world/canvas isolation, animated-portrait pixels, reset resource release and RTS restoration.");
}
