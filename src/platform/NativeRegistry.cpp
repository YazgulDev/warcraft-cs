#include "NativeRegistry.hpp"
#include <windows.h>
#include <cstring>
#include <vector>

namespace {
bool Read(uintptr_t address,void* value,size_t size) {
    SIZE_T got=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),value,size,&got) && got==size;
}
bool Matches(uintptr_t address,const char* expected,uintptr_t begin,uintptr_t end) {
    size_t size=std::strlen(expected)+1;char text[128]={};
    return size<=sizeof(text) && address>=begin && address<end && size<=end-address &&
        Read(address,text,size) && !std::memcmp(text,expected,size);
}
uint32_t Word(const unsigned char* p){uint32_t value=0;std::memcpy(&value,p,4);return value;}
}
uintptr_t NativeRegistry::Resolve(uintptr_t module,const char* name,const char* signature) {
    if(!module || !name || !signature)return 0;
    IMAGE_DOS_HEADER dos={};IMAGE_NT_HEADERS32 nt={};
    if(!Read(module,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 || dos.e_lfanew>0x100000 ||
        !Read(module+dos.e_lfanew,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC || nt.FileHeader.NumberOfSections>96 ||
        nt.OptionalHeader.SizeOfImage<4096 || nt.OptionalHeader.SizeOfImage>128*1024*1024)return 0;
    uintptr_t end=module+nt.OptionalHeader.SizeOfImage;if(end<=module)return 0;
    uintptr_t table=module+dos.e_lfanew+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
    if(table<module || table>end || nt.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER)>end-table)return 0;
    uintptr_t resolved=0;
    for(unsigned section=0;section<nt.FileHeader.NumberOfSections;++section) {
        IMAGE_SECTION_HEADER info={};if(!Read(table+section*sizeof(info),&info,sizeof(info)))return 0;
        if(!(info.Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
        size_t size=info.Misc.VirtualSize;
        if(info.VirtualAddress>=nt.OptionalHeader.SizeOfImage || size>nt.OptionalHeader.SizeOfImage-info.VirtualAddress)return 0;
        uintptr_t code=module+info.VirtualAddress;std::vector<unsigned char> bytes(size);
        if(size<20 || !Read(code,bytes.data(),bytes.size()))continue;
        for(size_t at=0;at+20<=size;++at) {
            const auto* p=bytes.data()+at;
            // Both classic builds register signature/name/implementation with this four-instruction record.
            if(p[0]!=0x68 || p[5]!=0xBA || p[10]!=0xB9 || p[15]!=0xE8 ||
                !Matches(Word(p+6),name,module,end) || !Matches(Word(p+1),signature,module,end))continue;
            uintptr_t function=Word(p+11);unsigned char prefix[3]={};
            if(function<code || function>=code+size-3 || !Read(function,prefix,sizeof(prefix)))continue;
            // The duplicate null-registration table is not an available implementation.
            if((prefix[0]==0x33 || prefix[0]==0x31) && prefix[1]==0xC0 && prefix[2]==0xC3)continue;
            if(resolved && resolved!=function)return 0; // Ambiguous patched registrations disable the capability.
            resolved=function;
        }
    }
    return resolved;
}
