#pragma once
#include <string>
#include <cstdint>

// Shared bounded text uses UTF-8; byte truncation must never split a Cyrillic character.
namespace UnicodeText {
inline uint32_t Next(const char*& p) {
    auto c=static_cast<unsigned char>(*p++);if(c<128)return c;
    int count=c>=0xF0 ? 3 : c>=0xE0 ? 2 : c>=0xC0 ? 1 : 0;
    uint32_t value=c&((1u<<(6-count))-1);
    if(!count)return '?';
    for(int i=0;i<count;++i){auto n=static_cast<unsigned char>(*p);if((n&0xC0)!=0x80)return '?';++p;value=(value<<6)|(n&63);}
    return value<=0x10FFFF && !(value>=0xD800 && value<=0xDFFF) ? value : '?';
}
inline void Append(std::string& text,uint32_t c) {
    if(c<32 || c==127 || c>0x10FFFF || (c>=0xD800 && c<=0xDFFF))return;
    if(c<128)text+=char(c);
    else if(c<2048){text+=char(0xC0|(c>>6));text+=char(0x80|(c&63));}
    else if(c<65536){text+=char(0xE0|(c>>12));text+=char(0x80|((c>>6)&63));text+=char(0x80|(c&63));}
    else {text+=char(0xF0|(c>>18));text+=char(0x80|((c>>12)&63));text+=char(0x80|((c>>6)&63));text+=char(0x80|(c&63));}
}
inline void Pop(std::string& text) {
    if(text.empty())return;size_t at=text.size()-1;
    while(at && (static_cast<unsigned char>(text[at])&0xC0)==0x80)--at;text.resize(at);
}
}
