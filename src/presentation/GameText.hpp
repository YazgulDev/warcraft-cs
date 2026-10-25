#pragma once
#include <cctype>
#include <string>

namespace GameText {
// Native map names may contain Warcraft color/newline escapes; preserve UTF-8 bytes, not UI control markup.
inline std::string Label(const std::string& text) {
    std::string result;
    for(size_t i=0;i<text.size() && result.size()<512;) {
        if(text[i]=='|' && i+1<text.size()) {
            if((text[i+1]=='c' || text[i+1]=='C') && i+10<=text.size()) {
                bool hex=true;for(size_t j=i+2;j<i+10;++j)hex&=std::isxdigit(static_cast<unsigned char>(text[j]))!=0;
                if(hex){i+=10;continue;}
            }
            if(text[i+1]=='r' || text[i+1]=='R'){i+=2;continue;}
            if(text[i+1]=='n' || text[i+1]=='N'){result+=' ';i+=2;continue;}
            if(text[i+1]=='|'){result+='|';i+=2;continue;}
        }
        unsigned char c=static_cast<unsigned char>(text[i++]);result+=c<32 || c==127 ? ' ' : char(c);
    }
    return result;
}
}
