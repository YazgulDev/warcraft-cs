#include "NativeChat.hpp"
#include "GameBuild.hpp"
#include "WarcraftApi.hpp"
#include <cstring>
#include <cstdint>

namespace NativeChat {
namespace {uintptr_t base=0;bool verified=false;
template<class T> bool Read(uintptr_t address,T& value){SIZE_T count=0;return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&value,sizeof(value),&count) && count==sizeof(value);}
}
bool Configure(uintptr_t gameBase){
    base=gameBase;verified=false;
    if(!base || wc3::ActiveBuild()!=wc3::GameBuildKind::Classic126a)return false;
    // Owned 1.26a ChatEditBar.SetText/GetText and GameUI.SubmitChat have independent ABI signatures.
    const unsigned char set[]={0x8B,0x44,0x24,0x04,0x8B,0x89,0xE0,0x01,0,0,0x6A,0x01,0x50};
    const unsigned char get[]={0x8B,0x89,0xE0,0x01,0,0,0xE9};
    const unsigned char submit[]={0x6A,0xFF,0x68};
    verified=!std::memcmp(reinterpret_cast<void*>(base+0x341410),set,sizeof(set)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x341450),get,sizeof(get)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x2FC700),submit,sizeof(submit));
    const unsigned char submitStack[]={0x81,0xEC,0xF8,0x01,0,0};
    verified=verified && !std::memcmp(reinterpret_cast<void*>(base+0x2FC70E),submitStack,sizeof(submitStack));
    wc3::Log("Native FPS chat adapter verified=%d",verified);return verified;
}
uint32_t Character(WPARAM value){
    if(value<128 || value>255)return uint32_t(value);
    char byte=char(value);wchar_t wide=0;
    return MultiByteToWideChar(CP_ACP,0,&byte,1,&wide,1)==1 ? uint32_t(wide) : '?';
}
std::wstring Clipboard(HWND window){
    if(!OpenClipboard(window))return {};std::wstring result;
    HANDLE data=GetClipboardData(CF_UNICODETEXT);auto text=data ? static_cast<const wchar_t*>(GlobalLock(data)) : nullptr;
    // Bound clipboard reads even when another application owns a very large string.
    if(text){size_t maximum=GlobalSize(data)/sizeof(wchar_t);for(size_t i=0;i<maximum && i<240 && text[i];++i)result+=text[i];GlobalUnlock(data);}CloseClipboard();return result;
}
bool Send(const std::string& utf8){
    if(!verified || utf8.empty() || utf8.size()>240 || !wc3::SinglePlayer())return false;
    using UI=uintptr_t (__fastcall*)(int,int);using Get=const char* (__thiscall*)(uintptr_t);
    using Set=void (__thiscall*)(uintptr_t,const char*);using Submit=int (__thiscall*)(uintptr_t,uintptr_t);
    auto ui=reinterpret_cast<UI>(base+0x300710)(0,0);uintptr_t chat=0,table=0;int editing=0;
    if(!Read(ui+0x3FC,chat) || !Read(chat,table) || table!=base+0x93EAC4 || !Read(chat+0x1DC,editing) || editing)return false;
    // Native edit text is UTF-8 already. Do not synthesize OS keys or overwrite an RTS chat draft.
    auto previous=reinterpret_cast<Get>(base+0x341450)(chat);if(previous && *previous)return false;
    reinterpret_cast<Set>(base+0x341410)(chat,utf8.c_str());
    auto actual=reinterpret_cast<Get>(base+0x341450)(chat);
    if(!actual || std::strcmp(actual,utf8.c_str())){reinterpret_cast<Set>(base+0x341410)(chat,"");return false;}
    return reinterpret_cast<Submit>(base+0x2FC700)(ui,0)!=0;
}
}
