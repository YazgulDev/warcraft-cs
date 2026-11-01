#include "../src/platform/NativeRegistry.hpp"
#include "../src/platform/ExplodedFlagLayout.hpp"
#include <windows.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <array>

int main() {
    _set_error_mode(_OUT_TO_STDERR);
    // This authored mapped PE contains only synthetic registry records and tiny non-executed code markers.
    auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x4000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(memory);
    uintptr_t base=reinterpret_cast<uintptr_t>(memory);
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(memory);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(memory+0x80);nt->Signature=IMAGE_NT_SIGNATURE;
    nt->FileHeader.NumberOfSections=1;nt->FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER32);
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR32_MAGIC;nt->OptionalHeader.SizeOfImage=0x4000;
    auto section=IMAGE_FIRST_SECTION(nt);section->VirtualAddress=0x1000;section->Misc.VirtualSize=0x1000;section->Characteristics=IMAGE_SCN_MEM_EXECUTE;
    std::memcpy(memory+0x3000,"WaygateIsActive",16);std::memcpy(memory+0x3040,"(Hunit;)B",10);
    memory[0x1800]=0x55;memory[0x1801]=0x8B;memory[0x1802]=0xEC;
    memory[0x1810]=0x53;memory[0x1811]=0x90;memory[0x1812]=0xC3;
    memory[0x1900]=0x33;memory[0x1901]=0xC0;memory[0x1902]=0xC3;
    auto record=[&](unsigned offset,unsigned implementation) {
        auto p=memory+offset;p[0]=0x68;p[5]=0xBA;p[10]=0xB9;p[15]=0xE8;
        uint32_t signature=uint32_t(base+0x3040),name=uint32_t(base+0x3000),function=uint32_t(base+implementation);
        std::memcpy(p+1,&signature,4);std::memcpy(p+6,&name,4);std::memcpy(p+11,&function,4);
    };
    record(0x1100,0x1800);record(0x1120,0x1900);
    assert(NativeRegistry::Resolve(base,"WaygateIsActive","(Hunit;)B")==base+0x1800);
    assert(!NativeRegistry::Resolve(base,"WaygateIsActive","(Hunit;)R"));
    assert(!NativeRegistry::Resolve(base,"Absent","(Hunit;)B"));
    record(0x1140,0x1810);assert(!NativeRegistry::Resolve(base,"WaygateIsActive","(Hunit;)B"));memory[0x1140]=0;
    memory[0x1100]=0;assert(!NativeRegistry::Resolve(base,"WaygateIsActive","(Hunit;)B"));record(0x1100,0x1800);
    section->Misc.VirtualSize=0x4000;assert(!NativeRegistry::Resolve(base,"WaygateIsActive","(Hunit;)B"));
    VirtualFree(memory,0,MEM_RELEASE);
    // The setter layout is accepted only with the right handle resolver and a unique paired flag change.
    std::array<unsigned char,96> code{};const unsigned char prefix[]={0x8B,0x4C,0x24,0x04,0x56,0xE8};
    std::memcpy(code.data(),prefix,6);code[10]=0x8B;code[11]=0xF0;
    int32_t relative=0x2000-0x100A;std::memcpy(code.data()+6,&relative,4);
    auto pair=[&](int at,int other) {
        code[at]=code[other]=0x81;code[at+1]=0x4E;code[other+1]=0x66;code[at+2]=code[other+2]=0x5C;
        uint32_t mask=0x2000,clear=~mask;std::memcpy(code.data()+at+3,&mask,4);std::memcpy(code.data()+other+3,&clear,4);
    };
    pair(32,48);auto layout=ExplodedFlagLayout::Decode(code.data(),code.size(),0x1000,0x2000);
    assert(layout.offset==0x5C && layout.mask==0x2000);
    assert(!ExplodedFlagLayout::Decode(code.data(),code.size(),0x1000,0x3000).mask);
    code[51]=0;assert(!ExplodedFlagLayout::Decode(code.data(),code.size(),0x1000,0x2000).mask);pair(32,48);
    pair(64,80);assert(!ExplodedFlagLayout::Decode(code.data(),code.size(),0x1000,0x2000).mask);
    std::puts("PASS exact native registry signatures, duplicate stubs, ambiguity/bounds and verified exploded-flag layout.");
}
