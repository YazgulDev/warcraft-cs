#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Owned model metadata selects real sequences. No stock/unit-specific animation indices are assumed.
class ModelMotionProfile {
public:
    struct Sequence {int index=-1;float seconds=1;std::string name;};
    Sequence stand,walk,run,takeoff,air,land;
    float walkSpeed=0,runSpeed=0;
    void Load(const std::vector<uint8_t>& bytes,const std::string& slk,const std::string& path,int type) {
        *this={};std::vector<Sequence> sequences;
        if(bytes.size()>=4 && !std::memcmp(bytes.data(),"MDLX",4))for(size_t at=4;at+8<=bytes.size();) {
            uint32_t length=0;std::memcpy(&length,bytes.data()+at+4,4);size_t body=at+8;
            if(length>bytes.size()-body)break;
            if(!std::memcmp(bytes.data()+at,"SEQS",4))for(size_t seq=body;seq+132<=body+length;seq+=132) {
                size_t n=0;while(n<80 && bytes[seq+n])++n;
                uint32_t begin=0,end=0;std::memcpy(&begin,bytes.data()+seq+80,4);std::memcpy(&end,bytes.data()+seq+84,4);
                if(end>begin && end-begin<=60000)sequences.push_back({int((seq-body)/132),float(end-begin)*.001f,std::string(reinterpret_cast<const char*>(bytes.data()+seq),n)});
            }
            at=body+length;
        }
        auto pick=[&](const std::string& token,bool fast=false) {
            Sequence selected;int best=1000;
            for(const auto& seq:sequences) {
                auto name=Lower(seq.name);
                bool match=name==token || name.rfind(token+" ",0)==0;
                if(fast)match=match || name.rfind("walk fast",0)==0;
                if(!match)continue;
                // Prefer normal locomotion over alternate, cinematic and injured variations.
                int score=int(name.size())+(name.find("alternate")!=std::string::npos ? 100 : 0);
                if(!fast && name.find("fast")!=std::string::npos)score+=100;
                if(score<best){selected=seq;best=score;}
            }
            return selected;
        };
        stand=pick("stand");walk=pick("walk");run=pick("run",true);
        if(walk.index<0)walk=pick("swim");if(walk.index<0)walk=pick("fly");
        if(run.index<0)run=walk;if(walk.index<0)walk=run;
        // Real Jump wins; safe native ready/hit/stand poses cover models without an authored leap.
        takeoff=pick("jump");air=takeoff;land=pick("land");
        if(takeoff.index<0)takeoff=pick("stand hit");
        if(air.index<0)air=pick("stand ready");
        if(land.index<0)land=pick("stand hit");
        if(takeoff.index<0)takeoff=stand;if(air.index<0)air=stand;if(land.index<0)land=stand;
        LoadSpeeds(slk,path,type);
    }
    static std::string Lower(std::string value) {
        for(auto& c:value){if(c=='/')c='\\';c=char(std::tolower(static_cast<unsigned char>(c)));}return value;
    }
private:
    static std::string ModelKey(std::string path) {
        path=Lower(path);path.resize(path.find(',')==std::string::npos ? path.size() : path.find(','));
        if(path.size()>4 && (path.substr(path.size()-4)==".mdx" || path.substr(path.size()-4)==".mdl"))path.resize(path.size()-4);
        return path;
    }
    void LoadSpeeds(const std::string& slk,const std::string& path,int type) {
        // UnitUI supplies authored animation speed, distinct from gameplay speed. Named columns may be reordered.
        std::unordered_map<int,std::unordered_map<int,std::string>> rows;int x=1,y=1;
        std::istringstream input(slk);std::string line;
        while(std::getline(input,line)) {
            if(line.rfind("C;",0)!=0)continue;
            std::istringstream cells(line.substr(2));std::string cell,value;bool known=false;
            while(std::getline(cells,cell,';')) {
                if(cell.empty())continue;
                if(cell[0]=='X')x=std::atoi(cell.c_str()+1);if(cell[0]=='Y')y=std::atoi(cell.c_str()+1);
                if(cell[0]=='K') {value=cell.substr(1);while(!value.empty() && (value.back()=='\r' || value.back()=='\n'))value.pop_back();
                    if(value.size()>=2 && value.front()=='"' && value.back()=='"')value=value.substr(1,value.size()-2);known=true;}
            }
            if(known && x>0 && x<128 && y>0 && y<20000)rows[y][x]=value;
        }
        int id=0,file=0,slow=0,fast=0;
        for(const auto& cell:rows[1]) {if(cell.second=="unitUIID")id=cell.first;if(cell.second=="file")file=cell.first;
            if(cell.second=="walk")slow=cell.first;if(cell.second=="run")fast=cell.first;}
        if(!id || !file || !slow || !fast)return;
        std::string raw(4,' ');for(int i=0;i<4;++i)raw[i]=char((uint32_t(type)>>(24-8*i))&255);
        const std::unordered_map<int,std::string>* chosen=nullptr;
        for(const auto& row:rows) {
            auto key=row.second.find(id),model=row.second.find(file);
            if(key!=row.second.end() && key->second==raw){chosen=&row.second;break;}
            if(!chosen && model!=row.second.end() && ModelKey(model->second)==ModelKey(path))chosen=&row.second;
        }
        if(!chosen)return;
        auto number=[&](int column) {auto at=chosen->find(column);if(at==chosen->end())return 0.f;
            char* end=nullptr;float value=std::strtof(at->second.c_str(),&end);
            return end!=at->second.c_str() && !*end && std::isfinite(value) && value>0 && value<=10000 ? value : 0.f;};
        walkSpeed=number(slow);runSpeed=number(fast);
    }
};
