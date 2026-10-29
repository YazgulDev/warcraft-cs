#include "../src/platform/D3DNativeUi.hpp"
#include "../src/presentation/D3DGraphics.hpp"
#include <cstdio>
#include <cstdlib>

namespace wc3 {void Log(const char*,...) {}}
namespace {
void Require(bool condition,const char* message) {if(!condition){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
struct Vertex {float x,y,z,w;DWORD color;};
void Triangle(IDirect3DDevice9* device,float x,float y,DWORD color) {
    Vertex vertices[]={{x,y,0,1,color},{x+36,y,0,1,color},{x,y+36,0,1,color}};
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);device->SetTexture(0,nullptr);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_ZENABLE,FALSE);
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
    Require(portrait.X==0 && portrait.Y==0,"portrait redirects into private local viewport");device->SetViewport(&portrait);Triangle(device,0,0,0xFFFFFF00);
    // The real engine changes projection after portraits and can omit another SetViewport call.
    D3DNativeUi::Projection(device,true);D3DVIEWPORT9 full={0,0,320,240,0,1};device->SetViewport(&full);
    Triangle(device,70,60,0xFFFF0000);device->EndScene();
    HDC dc=GetDC(window);D3DNativeUi::Overlay(true);Require(D3DGraphics::BeginFrame(device,dc,320,240),"portrait canvas");
    D3DGraphics::MatrixMode(GL_PROJECTION);D3DGraphics::LoadIdentity();D3DGraphics::Ortho(0,320,240,0,-1,1);
    D3DGraphics::MatrixMode(GL_MODELVIEW);D3DGraphics::LoadIdentity();D3DNativeUi::DrawPortrait(200,60,40);D3DGraphics::EndFrame();D3DNativeUi::Overlay(false);
    Require(Pixel(device,15,15)==0x0000FF,"world pixels survive native console clear");
    Require(Pixel(device,75,15)==0x101010,"native cursor and HP geometry suppressed in the world region");
    Require(Pixel(device,135,15)==0x00FF00,"own overlay geometry remains visible");
    Require(Pixel(device,205,65)==0xFFFF00,"actual native portrait pixels reach the HUD");
    // Device Reset must succeed after releasing every capture surface, and RTS must resume native drawing.
    D3DNativeUi::BeforeReset(device);D3DNativeUi::Configure(false,0);D3DNativeUi::Phase(false);
    Require(SUCCEEDED(device->Reset(&params)),"capture does not retain default-pool surfaces across Reset");
    device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);device->BeginScene();Triangle(device,10,10,0xFFFF0000);device->EndScene();
    Require(Pixel(device,15,15)==0xFF0000,"native RTS geometry restored");
    ReleaseDC(window,dc);device->Release();api->Release();DestroyWindow(window);
    std::puts("PASS native D3D UI filtering, all draw forms, world/canvas isolation, animated-portrait pixels, reset resource release and RTS restoration.");
}
