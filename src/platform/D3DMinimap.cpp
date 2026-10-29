#include "D3DMinimap.hpp"
#include "WarcraftApi.hpp"
#include "../presentation/D3DGraphics.hpp"
#include "../presentation/GraphicsDiagnostics.hpp"

namespace D3DMinimap {
namespace {
IDirect3DDevice9* owner=nullptr;
IDirect3DTexture9* map=nullptr;
IDirect3DSurface9* uiColor=nullptr,*uiDepth=nullptr,*mapSurface=nullptr,*worldColor=nullptr,*worldDepth=nullptr;
RECT bounds={};int width=0,height=0,allocatedWidth=0,allocatedHeight=0,mapWidth=0,mapHeight=0;
D3DVIEWPORT9 worldViewport={};
bool enabled=false,capturing=false,ready=false;
template<class T>void Release(T*& resource) {if(resource)resource->Release();resource=nullptr;}
void RestoreWorld() {
    owner->SetDepthStencilSurface(nullptr);owner->SetRenderTarget(0,worldColor);owner->SetDepthStencilSurface(worldDepth);
    capturing=false;owner->SetViewport(&worldViewport);Release(worldColor);Release(worldDepth);
}
void Dispose() {
    FinishFrame();Release(mapSurface);Release(map);Release(uiColor);Release(uiDepth);
    ready=false;allocatedWidth=allocatedHeight=mapWidth=mapHeight=0;
}
bool Prepare() {
    int cropWidth=bounds.right-bounds.left,cropHeight=bounds.bottom-bounds.top;
    if(uiColor && allocatedWidth==width && allocatedHeight==height && mapWidth==cropWidth && mapHeight==cropHeight)return true;
    Dispose();IDirect3DSurface9* target=nullptr;D3DSURFACE_DESC desc={};
    if(!GraphicsDiagnostics::Result("Minimap.GetTarget",owner->GetRenderTarget(0,&target)))return false;
    HRESULT result=target->GetDesc(&desc);target->Release();
    if(SUCCEEDED(result))result=owner->CreateRenderTarget(width,height,desc.Format,D3DMULTISAMPLE_NONE,0,FALSE,&uiColor,nullptr);
    if(SUCCEEDED(result))result=owner->CreateDepthStencilSurface(width,height,D3DFMT_D16,D3DMULTISAMPLE_NONE,0,TRUE,&uiDepth,nullptr);
    if(SUCCEEDED(result))result=owner->CreateTexture(cropWidth,cropHeight,1,D3DUSAGE_RENDERTARGET,desc.Format,D3DPOOL_DEFAULT,&map,nullptr);
    if(SUCCEEDED(result))result=map->GetSurfaceLevel(0,&mapSurface);
    if(!GraphicsDiagnostics::Result("Minimap.CreateTargets",result,width,height)) {Dispose();return false;}
    allocatedWidth=width;allocatedHeight=height;mapWidth=cropWidth;mapHeight=cropHeight;
    wc3::Log("D3D9 minimap native crop=%ld,%ld %dx%d canvas=%dx%d",bounds.left,bounds.top,mapWidth,mapHeight,width,height);return true;
}
}
void Attach(IDirect3DDevice9* device) {if(owner!=device) {Dispose();owner=device;}}
void Configure(const RECT& rectangle,int frameWidth,int frameHeight,bool value) {
    // The same evaluated Warcraft CMinimap rectangle feeds GL and D3D, including custom/rectangular maps.
    bool valid=value && frameWidth>0 && frameHeight>0 && rectangle.left>=0 && rectangle.top>=0 &&
        rectangle.right<=frameWidth && rectangle.bottom<=frameHeight && rectangle.right>rectangle.left && rectangle.bottom>rectangle.top;
    if(!valid || frameWidth!=width || frameHeight!=height || !EqualRect(&rectangle,&bounds))Dispose();
    enabled=valid;bounds=rectangle;width=frameWidth;height=frameHeight;ready=false;
}
bool BeginNativeUi(IDirect3DDevice9* device) {
    if(device!=owner || !enabled)return false;
    if(!capturing) {
        if(!Prepare() || !GraphicsDiagnostics::Result("Minimap.SaveWorld",device->GetRenderTarget(0,&worldColor)))return false;
        device->GetDepthStencilSurface(&worldDepth);device->GetViewport(&worldViewport);
        device->SetDepthStencilSurface(nullptr);
        HRESULT result=device->SetRenderTarget(0,uiColor);
        if(SUCCEEDED(result))result=device->SetDepthStencilSurface(uiDepth);
        if(FAILED(result)) {
            GraphicsDiagnostics::Result("Minimap.BindUi",result);device->SetRenderTarget(0,worldColor);device->SetDepthStencilSurface(worldDepth);
            Release(worldColor);Release(worldDepth);return false;
        }
        // A private full-size UI target preserves Warcraft's native projection and scissor coordinates.
        capturing=true;
        D3DVIEWPORT9 full={0,0,DWORD(width),DWORD(height),0,1};device->SetViewport(&full);
        device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFF000000,1,0);
        device->SetViewport(&worldViewport);
    }
    // The old band clip deliberately excluded the RTS minimap; private UI pixels may cover its full rectangle.
    device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);return true;
}
bool Capturing(IDirect3DDevice9* device) {return device==owner && capturing;}
void FinishFrame() {
    if(!capturing)return;
    // StretchRect remains GPU-only. World color/depth are restored before health bars, weapons and HUD composition.
    RestoreWorld();
    ready=GraphicsDiagnostics::Result("Minimap.CopyCrop",owner->StretchRect(uiColor,&bounds,mapSurface,nullptr,D3DTEXF_NONE));
}
void BeginWorld() {
    // A second world submission can precede Present. Release its private UI target without a copy inside BeginScene.
    if(capturing) {RestoreWorld();ready=false;}
}
void BeforeReset(IDirect3DDevice9* device) {if(device==owner)Dispose();}
void Draw(const MinimapLayout& layout) {
    if(ready && enabled && map && layout.side>0)D3DGraphics::DrawTexture(map,layout.x,layout.y,layout.side,layout.side);
}
}
