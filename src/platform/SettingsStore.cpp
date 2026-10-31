#include "SettingsStore.hpp"
#include "WarcraftApi.hpp"
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cstring>

namespace {
bool Stem(const std::string& s){return !s.empty() && s.size()<=48 && s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==std::string::npos;}
bool Allowed(const ConfigChange& c) {
    if(c.value.empty() || c.value.find_first_of("\r\n")!=std::string::npos)return false;
    if(c.section=="Mouse")return c.key=="Sensitivity" || c.key=="ScopeSensitivity";
    if(c.section=="Audio")return c.key=="CSVolumePercent";
    if(c.section=="Rendering")return c.key=="DrawDistance";
    if(c.section=="Movement")return c.key=="Mode" && (c.value=="classic" || c.value=="recovery" || c.value=="hull");
    if(c.section!="Sky")return false;
    if(c.key=="Visible" || c.key=="Enabled" || c.key=="WarcraftEnabled")return c.value=="true" || c.value=="false";
    return (c.key=="Default" || (c.key.size()==1 && std::strchr("ALFWBYXVQJNDCIGOZ",c.key[0]))) && Stem(c.value);
}
}
bool SettingsStore::Save(const std::string& filename,const std::vector<ConfigChange>& changes) {
    if(changes.empty())return true;
    if(std::any_of(changes.begin(),changes.end(),[](const auto& c){return !Allowed(c);}))return false;
    auto path=std::filesystem::absolute(filename);char temporary[MAX_PATH]={};
    if(!GetTempFileNameA(path.parent_path().string().c_str(),"wcs",0,temporary))return false;
    bool exists=GetFileAttributesA(path.string().c_str())!=INVALID_FILE_ATTRIBUTES;
    bool okay=!exists || CopyFileA(path.string().c_str(),temporary,FALSE)!=FALSE;
    // Flush the same-directory temporary file before replacing the original, preserving its comments and all other keys.
    for(const auto& change:changes)if(okay)okay=WritePrivateProfileStringA(change.section.c_str(),change.key.c_str(),change.value.c_str(),temporary)!=FALSE;
    // A cache-flush call can return zero despite success; the following real file flush is authoritative.
    if(okay)WritePrivateProfileStringA(nullptr,nullptr,nullptr,temporary);
    if(okay) {
        HANDLE file=CreateFileA(temporary,GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        okay=file!=INVALID_HANDLE_VALUE;if(okay){okay=FlushFileBuffers(file)!=FALSE;CloseHandle(file);}
    }
    if(okay)okay=exists ? ReplaceFileA(path.string().c_str(),temporary,nullptr,REPLACEFILE_IGNORE_MERGE_ERRORS,nullptr,nullptr)!=FALSE :
        MoveFileExA(temporary,path.string().c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
    DWORD error=okay ? 0 : GetLastError();if(!okay){SetFileAttributesA(temporary,FILE_ATTRIBUTE_NORMAL);DeleteFileA(temporary);}
    wc3::Log("In-game settings save success=%d changedKeys=%u error=%lu",okay,unsigned(changes.size()),error);return okay;
}
std::vector<std::string> SettingsStore::Skies(const std::string& root) {
    std::vector<std::string> names;std::error_code error;
    auto path=std::filesystem::path(root)/"assets"/"skies";
    for(std::filesystem::directory_iterator it(path,error),end;!error && it!=end;it.increment(error)) {
        if(!it->is_regular_file(error) || it->path().extension()!=".wcs" || !Stem(it->path().stem().string()))continue;
        std::ifstream file(it->path(),std::ios::binary);char magic[4];uint32_t w=0,h=0;
        if(!file.read(magic,4) || std::memcmp(magic,"WCS1",4) || !file.read(reinterpret_cast<char*>(&w),4) || !file.read(reinterpret_cast<char*>(&h),4) || !w || w!=h || w>2048)continue;
        file.seekg(0,std::ios::end);if(file.tellg()!=std::streamoff(12+uint64_t(w)*h*24))continue;
        names.push_back(it->path().stem().string());
    }
    std::sort(names.begin(),names.end());return names;
}
