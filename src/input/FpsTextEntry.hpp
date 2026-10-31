#pragma once
#include "UnicodeText.hpp"
#include <deque>
#include <algorithm>

// Window messages edit only local text; submissions are consumed by the simulation thread.
class FpsTextEntry {
public:
    static constexpr size_t MaximumBytes=240;
    void Open(){if(!open_){buffer_.clear();open_=true;transition_=true;}}
    void Cancel(){if(open_){open_=false;buffer_.clear();transition_=true;}}
    void Character(uint32_t c){if(!open_)return;if(c==8){UnicodeText::Pop(buffer_);return;}auto next=buffer_;UnicodeText::Append(next,c);if(next.size()<=MaximumBytes)buffer_=next;}
    void Submit(){if(!open_)return;if(!buffer_.empty() && pending_.size()<8)pending_.push_back(buffer_);Cancel();}
    std::string Take(){if(pending_.empty())return {};auto text=pending_.front();pending_.pop_front();return text;}
    bool Transition(){bool result=transition_;transition_=false;return result;}
    bool Opened() const{return open_;}
    const std::string& Buffer() const{return buffer_;}
    void Reset(){open_=transition_=false;buffer_.clear();pending_.clear();}
    static bool NuclearCode(std::string text,const std::string& code){
        // Exact whole-line comparison (case insensitive) prevents ordinary sentences from granting a bomb.
        auto first=text.find_first_not_of(" \t"),last=text.find_last_not_of(" \t");
        if(first==std::string::npos || code.empty())return false;text=text.substr(first,last-first+1);
        auto upper=[](unsigned char c){return c>='a' && c<='z' ? char(c-'a'+'A') : char(c);};
        std::transform(text.begin(),text.end(),text.begin(),upper);auto expected=code;std::transform(expected.begin(),expected.end(),expected.begin(),upper);return text==expected;
    }
private:
    bool open_=false,transition_=false;
    std::string buffer_;std::deque<std::string> pending_;
};
