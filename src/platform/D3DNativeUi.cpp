#include "D3DNativeUi.hpp"
#include "WarcraftApi.hpp"
#include "../presentation/D3DGraphics.hpp"
#include "../presentation/GraphicsDiagnostics.hpp"

namespace D3DNativeUi {
namespace {
using Primitive=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,UINT);
using Indexed=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,INT,UINT,UINT,UINT,UINT);
using PrimitiveUP=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,const void*,UINT);
using IndexedUP=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,UINT,UINT,const void*,D3DFORMAT,const void*,UINT);
using Clear=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,DWORD,const D3DRECT*,DWORD,D3DCOLOR,float,DWORD);
using Cursor=BOOL (STDMETHODCALLTYPE*)(IDirect3DDevice9*,BOOL);
struct Dispatch {void** table=nullptr;Primitive primitive=nullptr;Indexed indexed=nullptr;PrimitiveUP up=nullptr;IndexedUP indexedUp=nullptr;Clear clear=nullptr;Cursor cursor=nullptr;};
Dispatch dispatch[4];
IDirect3DDevice9* owner=nullptr;
IDirect3DTexture9* texture=nullptr;
IDirect3DSurface9* color=nullptr,*depth=nullptr,*previousColor=nullptr,*previousDepth=nullptr;
bool enabled=false,nativeUi=false,overlay=false,capturing=false,drawn=false,ready=false;
bool hideCursor=false,engineCursor=false;
uint32_t actor=0;int cropWidth=0,cropHeight=0;
unsigned rejected=0,portraitDraws=0;
template<class T>void Release(T*& resource) {if(resource)resource->Release();resource=nullptr;}
Dispatch* Find(IDirect3DDevice9* device) {auto table=*reinterpret_cast<void***>(device);for(auto& slot:dispatch)if(slot.table==table)return &slot;return nullptr;}
bool Skip(IDirect3DDevice9* device) {
    if(device!=owner || overlay || !enabled)return false;
    if(capturing) {
        // Clear after the engine bound the local viewport, and before its first portrait batch.
        if(!drawn)Find(device)->clear(device,0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFF000000,1,0);
        drawn=true;++portraitDraws;device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);return false;
    }
    if(nativeUi) {++rejected;return true;}return false;
}
HRESULT STDMETHODCALLTYPE Draw(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT start,UINT count) {auto f=Find(d);return f ? (Skip(d) ? D3D_OK : f->primitive(d,t,start,count)) : D3DERR_INVALIDCALL;}
HRESULT STDMETHODCALLTYPE DrawIndexed(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,INT base,UINT min,UINT vertices,UINT start,UINT count) {auto f=Find(d);return f ? (Skip(d) ? D3D_OK : f->indexed(d,t,base,min,vertices,start,count)) : D3DERR_INVALIDCALL;}
HRESULT STDMETHODCALLTYPE DrawUP(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT count,const void* data,UINT stride) {auto f=Find(d);return f ? (Skip(d) ? D3D_OK : f->up(d,t,count,data,stride)) : D3DERR_INVALIDCALL;}
HRESULT STDMETHODCALLTYPE DrawIndexedUP(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT min,UINT vertices,UINT count,const void* indices,D3DFORMAT format,const void* data,UINT stride) {auto f=Find(d);return f ? (Skip(d) ? D3D_OK : f->indexedUp(d,t,min,vertices,count,indices,format,data,stride)) : D3DERR_INVALIDCALL;}
HRESULT STDMETHODCALLTYPE ClearTarget(IDirect3DDevice9* d,DWORD count,const D3DRECT* rects,DWORD flags,D3DCOLOR colorValue,float z,DWORD stencil) {
    auto f=Find(d);if(!f)return D3DERR_INVALIDCALL;
    // Console clears must not erase the expanded world. Portrait clears cover only its private target.
    if(d==owner && enabled && !overlay) {if(capturing)return f->clear(d,0,nullptr,flags,colorValue,z,stencil);if(nativeUi)return D3D_OK;}
    return f->clear(d,count,rects,flags,colorValue,z,stencil);
}
BOOL STDMETHODCALLTYPE ShowCursor(IDirect3DDevice9* device,BOOL visible) {
    auto f=Find(device);if(!f)return FALSE;
    if(device!=owner)return f->cursor(device,visible);
    // Preserve the game's requested visibility while FPS temporarily owns the hardware cursor.
    BOOL previous=engineCursor;engineCursor=visible!=FALSE;f->cursor(device,engineCursor && !hideCursor);return previous;
}
void Dispose() {Finish();Release(color);Release(depth);Release(texture);ready=false;cropWidth=cropHeight=0;}
bool Prepare(IDirect3DDevice9* device,int width,int height) {
    if(texture && cropWidth==width && cropHeight==height)return true;
    Dispose();D3DSURFACE_DESC desc={};IDirect3DSurface9* back=nullptr;
    if(!GraphicsDiagnostics::Result("NativePortrait.GetTarget",device->GetRenderTarget(0,&back)))return false;back->GetDesc(&desc);back->Release();
    HRESULT result=device->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,desc.Format,D3DPOOL_DEFAULT,&texture,nullptr);
    if(SUCCEEDED(result))result=texture->GetSurfaceLevel(0,&color);
    if(SUCCEEDED(result))result=device->CreateDepthStencilSurface(width,height,D3DFMT_D16,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr);
    if(!GraphicsDiagnostics::Result("NativePortrait.CreateTarget",result,width,height)) {Dispose();return false;}
    cropWidth=width;cropHeight=height;wc3::Log("D3D9 portrait target=%dx%d",width,height);return true;
}
}
void Attach(IDirect3DDevice9* device) {
    bool changed=owner!=device;
    if(changed) {Dispose();owner=device;}
    if(auto existing=Find(device)) {
        // A replacement device can reuse its runtime's already patched table, but has its own cursor state.
        if(changed) {engineCursor=existing->cursor(device,FALSE)!=FALSE;existing->cursor(device,engineCursor && !hideCursor);}
        return;
    }
    auto table=*reinterpret_cast<void***>(device);Dispatch* slot=nullptr;for(auto& candidate:dispatch)if(!candidate.table){slot=&candidate;break;}if(!slot)return;
    // Patch COM slots rather than detouring driver-generated draw thunks by their machine-code address.
    DWORD protection=0;if(!VirtualProtect(table+12,(85-12)*sizeof(void*),PAGE_READWRITE,&protection))return;
    *slot={table,reinterpret_cast<Primitive>(table[81]),reinterpret_cast<Indexed>(table[82]),reinterpret_cast<PrimitiveUP>(table[83]),reinterpret_cast<IndexedUP>(table[84]),reinterpret_cast<Clear>(table[43]),reinterpret_cast<Cursor>(table[12])};
    engineCursor=slot->cursor(device,FALSE)!=FALSE;slot->cursor(device,engineCursor && !hideCursor);
    table[12]=reinterpret_cast<void*>(&ShowCursor);table[43]=reinterpret_cast<void*>(&ClearTarget);table[81]=reinterpret_cast<void*>(&Draw);table[82]=reinterpret_cast<void*>(&DrawIndexed);table[83]=reinterpret_cast<void*>(&DrawUP);table[84]=reinterpret_cast<void*>(&DrawIndexedUP);
    DWORD ignored=0;VirtualProtect(table+12,(85-12)*sizeof(void*),protection,&ignored);
    wc3::Log("D3D9 native UI submission filter attached device=%p",device);
}
void Configure(bool value,uint32_t unit,bool cursor) {
    bool hidden=value && cursor;
    if(hideCursor!=hidden) {
        hideCursor=hidden;if(owner)if(auto f=Find(owner))f->cursor(owner,engineCursor && !hideCursor);
        wc3::Log("D3D9 FPS hardware cursor hidden=%d nativeRequested=%d",hideCursor,engineCursor);
    }
    if(actor!=unit) {Finish();ready=false;}
    enabled=value;actor=unit;
    // RTS/menu/unarmed views release their portrait surfaces instead of retaining an old actor image.
    if(!enabled || !actor)Dispose();
}
void Phase(bool value) {nativeUi=value;}
void Overlay(bool value) {overlay=value;}
void Projection(IDirect3DDevice9* device,bool orthographic) {
    // Warcraft can resume fullscreen UI by changing only its projection, without reissuing a viewport.
    if(device==owner && !overlay && orthographic)Finish();
}
void Viewport(IDirect3DDevice9* device,D3DVIEWPORT9& viewport,int width,int height) {
    if(device!=owner || overlay)return;Finish();
    // Native portrait viewports are small, bottom-console 3D regions; the world and menu previews stay untouched.
    if(!enabled || !actor || !nativeUi || viewport.Width<16 || viewport.Height<16 ||
        viewport.Width>width*.25f || viewport.Height>height*.35f || viewport.X<width*.15f ||
        viewport.X>width*.55f || viewport.Y<height*.65f || int(viewport.Y)>height)return;
    if(!Prepare(device,viewport.Width,viewport.Height))return;
    if(FAILED(device->GetRenderTarget(0,&previousColor)))return;
    device->GetDepthStencilSurface(&previousDepth);
    if(FAILED(device->SetDepthStencilSurface(nullptr)) || FAILED(device->SetRenderTarget(0,color)) || FAILED(device->SetDepthStencilSurface(depth))) {
        device->SetRenderTarget(0,previousColor);device->SetDepthStencilSurface(previousDepth);Release(previousColor);Release(previousDepth);return;
    }
    capturing=true;drawn=false;viewport.X=viewport.Y=0;
}
void Finish() {
    if(!capturing)return;
    // Restore the engine targets before its next viewport, Present or Reset; default-pool resources never survive Reset.
    owner->SetDepthStencilSurface(nullptr);owner->SetRenderTarget(0,previousColor);owner->SetDepthStencilSurface(previousDepth);
    Release(previousColor);Release(previousDepth);capturing=false;if(drawn)ready=true;
}
void BeforeReset(IDirect3DDevice9* device) {if(device==owner)Dispose();}
void DrawPortrait(int x,int y,int side) {
    if(ready && enabled && actor && texture)D3DGraphics::DrawTexture(texture,x,y,side,side);
    static DWORD logged=0;if(enabled && GetTickCount()-logged>5000) {logged=GetTickCount();wc3::Log("D3D9 native UI rejected=%u portraitDraws=%u ready=%d",rejected,portraitDraws,ready);}
}
}
