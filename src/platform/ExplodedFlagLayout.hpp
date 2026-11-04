#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

// Decode only the verified public setter's paired OR/AND on the resolved CUnit, not a global unit layout.
struct ExplodedFlagLayout {
    unsigned offset=0;uint32_t mask=0;
    static ExplodedFlagLayout Decode(const unsigned char* bytes,size_t size,uintptr_t function,uintptr_t resolver) {
        const unsigned char prefix[]={0x8B,0x4C,0x24,0x04,0x56,0xE8};
        if(size<32 || std::memcmp(bytes,prefix,sizeof(prefix)) || bytes[10]!=0x8B || bytes[11]!=0xF0)return {};
        int32_t relative=0;std::memcpy(&relative,bytes+6,4);
        if(uintptr_t(function+10+relative)!=resolver)return {};
        ExplodedFlagLayout result;
        for(size_t at=12;at+7<=size;++at)if(bytes[at]==0x81 && bytes[at+1]==0x4E && bytes[at+2]<128) {
            uint32_t mask=0;std::memcpy(&mask,bytes+at+3,4);
            if(!mask || (mask&(mask-1)))continue;
            for(size_t other=at+7;other+7<=size;++other)if(bytes[other]==0x81 && bytes[other+1]==0x66 && bytes[other+2]==bytes[at+2]) {
                uint32_t cleared=0;std::memcpy(&cleared,bytes+other+3,4);
                if(cleared!=~mask)continue;
                if(result.mask)return {}; // A second candidate is not a verified ownership flag.
                result={bytes[at+2],mask};
            }
        }
        return result;
    }
};
