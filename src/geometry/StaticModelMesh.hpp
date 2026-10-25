#pragma once
#include "CylinderMesh.hpp"
#include <cstring>

// Decode only a verified, stationary Stand pose. Unsupported versions, animation or
// materials keep the existing conservative volume instead of granting an unsafe passage.
namespace StaticModelMesh {
inline unsigned UInt(const unsigned char* data) {unsigned value;std::memcpy(&value,data,4);return value;}
inline float Float(const unsigned char* data) {float value;std::memcpy(&value,data,4);return value;}
inline bool Array(const std::vector<unsigned char>& bytes,size_t end,size_t& cursor,const char* tag,
    size_t stride,size_t& data,unsigned& count) {
    if(cursor>end || end-cursor<8 || std::memcmp(bytes.data()+cursor,tag,4))return false;
    count=UInt(bytes.data()+cursor+4);data=cursor+8;
    if(count>(end-data)/stride)return false;
    cursor=data+count*stride;return true;
}
inline bool IdentityTracks(const std::vector<unsigned char>& bytes,size_t at,size_t end,unsigned start,unsigned finish) {
    while(at<end) {
        if(end-at<16)return false;
        bool rotation=!std::memcmp(bytes.data()+at,"KGRT",4),scale=!std::memcmp(bytes.data()+at,"KGSC",4);
        if(!rotation && !scale && std::memcmp(bytes.data()+at,"KGTR",4))return false;
        unsigned count=UInt(bytes.data()+at+4),interpolation=UInt(bytes.data()+at+8),global=UInt(bytes.data()+at+12);
        unsigned components=rotation ? 4 : 3,values=interpolation>1 ? 3 : 1,stride=4+components*4*values;
        at+=16;
        if(interpolation>3 || global!=0xffffffff || count>(end-at)/stride)return false;
        // Warcraft samples tracks inside the active sequence interval. Death-only helper
        // tracks do not deform Stand; any non-identity key in Stand requires the fallback.
        for(unsigned i=0;i<count;++i) {
            size_t key=at+size_t(i)*stride;unsigned time=UInt(bytes.data()+key);
            if(time<start || time>finish)continue;
            for(unsigned v=0;v<values;++v)for(unsigned c=0;c<components;++c) {
                float expected=scale || (rotation && c==3) ? 1.f : 0.f;
                // Tangents in cubic tracks are not interpreted as static poses.
                float value=Float(bytes.data()+key+4+(v*components+c)*4);
                if(!std::isfinite(value) || (v==0 && std::abs(value-expected)>.0001f) || (v>0 && interpolation>1))return false;
            }
        }
        at+=size_t(count)*stride;
    }
    return true;
}
inline bool Load(const std::vector<unsigned char>& bytes,CylinderMesh& result) {
    result.triangles.clear();
    if(bytes.size()<4 || std::memcmp(bytes.data(),"MDLX",4))return false;
    struct Chunk {size_t begin,end;};
    std::vector<Chunk> geosets,animations,nodes;std::vector<bool> solidMaterials;
    unsigned standStart=0,standEnd=0;bool stand=false,version=false;
    for(size_t at=4;at<bytes.size();) {
        if(bytes.size()-at<8)return false;
        unsigned length=UInt(bytes.data()+at+4);size_t body=at+8,end=body+length;
        if(length>bytes.size()-body)return false;
        const auto* tag=bytes.data()+at;
        if(!std::memcmp(tag,"VERS",4))version=length==4 && UInt(bytes.data()+body)==800;
        if(!std::memcmp(tag,"SEQS",4))for(size_t seq=body;seq+132<=end;seq+=132)
            if(!stand && !std::memcmp(bytes.data()+seq,"Stand",5)) {
                standStart=UInt(bytes.data()+seq+80);standEnd=UInt(bytes.data()+seq+84);stand=true;
            }
        if(!std::memcmp(tag,"GEOS",4))geosets.push_back({body,end});
        if(!std::memcmp(tag,"GEOA",4))animations.push_back({body,end});
        if(!std::memcmp(tag,"BONE",4) || !std::memcmp(tag,"HELP",4))nodes.push_back({body,end});
        if(!std::memcmp(tag,"MTLS",4))for(size_t material=body;material<end;) {
            if(end-material<20)return false;
            unsigned size=UInt(bytes.data()+material);
            if(size<20 || size>end-material || std::memcmp(bytes.data()+material+12,"LAYS",4))return false;
            unsigned count=UInt(bytes.data()+material+16);size_t layer=material+20;bool solid=false;
            for(unsigned i=0;i<count;++i) {
                if(material+size-layer<28)return false;
                unsigned layerSize=UInt(bytes.data()+layer),filter=UInt(bytes.data()+layer+4);
                if(layerSize<28 || layerSize>material+size-layer || filter>6)return false;
                float alpha=Float(bytes.data()+layer+24);
                // Opaque and alpha-cutout surfaces collide; additive light/glow cards do not.
                solid=solid || (filter<=1 && std::isfinite(alpha) && alpha>.01f);
                if(layerSize!=28)return false;
                layer+=layerSize;
            }
            if(layer!=material+size)return false;
            solidMaterials.push_back(solid);material+=size;
        }
        at=end;
    }
    if(!version || !stand || standEnd<standStart)return false;
    for(const auto& chunk:nodes)for(size_t at=chunk.begin;at<chunk.end;) {
        if(chunk.end-at<96)return false;
        unsigned size=UInt(bytes.data()+at);
        if(size<96 || size>chunk.end-at || !IdentityTracks(bytes,at+96,at+size,standStart,standEnd))return false;
        // BONE has two extra IDs following the generic node; HELP has only the node.
        bool bone=chunk.begin>=8 && !std::memcmp(bytes.data()+chunk.begin-8,"BONE",4);
        if(bone && chunk.end-at-size<8)return false;
        at+=size+(bone ? 8 : 0);
    }
    std::vector<bool> visible;
    for(const auto& chunk:animations)for(size_t at=chunk.begin;at<chunk.end;) {
        if(chunk.end-at<28)return false;
        unsigned size=UInt(bytes.data()+at),id=UInt(bytes.data()+at+24);
        if(size<28 || size>chunk.end-at || id>65535)return false;
        float alpha=Float(bytes.data()+at+4);
        if(!std::isfinite(alpha))return false;
        size_t cursor=at+28;
        if(cursor<at+size) {
            if(at+size-cursor<16 || std::memcmp(bytes.data()+cursor,"KGAO",4))return false;
            unsigned count=UInt(bytes.data()+cursor+4),interpolation=UInt(bytes.data()+cursor+8);
            if(interpolation>1 || UInt(bytes.data()+cursor+12)!=0xffffffff || count>(at+size-cursor-16)/8)return false;
            cursor+=16;
            for(unsigned i=0;i<count;++i) {
                unsigned time=UInt(bytes.data()+cursor+i*8);float value=Float(bytes.data()+cursor+i*8+4);
                if(!std::isfinite(value))return false;
                if(time<=standStart)alpha=value;
                if(time>standStart && time<=standEnd && std::abs(value-alpha)>.0001f)return false;
            }
            cursor+=count*8;
        }
        if(cursor!=at+size)return false;
        if(visible.size()<=id)visible.resize(id+1,true);
        visible[id]=alpha>.01f;at+=size;
    }
    CylinderMesh candidate;unsigned geosetId=0;
    for(const auto& chunk:geosets)for(size_t at=chunk.begin;at<chunk.end;++geosetId) {
        if(chunk.end-at<4)return false;
        unsigned size=UInt(bytes.data()+at);if(size<4 || size>chunk.end-at)return false;
        size_t end=at+size,cursor=at+4,data[8];unsigned count[8];
        const char* tags[]={"VRTX","NRMS","PTYP","PCNT","PVTX","GNDX","MTGC","MATS"};
        const unsigned strides[]={12,12,4,4,2,1,4,4};
        for(unsigned i=0;i<8;++i)if(!Array(bytes,end,cursor,tags[i],strides[i],data[i],count[i]))return false;
        if(!count[0] || count[0]>65536 || count[1]!=count[0] || count[4]%3 || end-cursor<12)return false;
        for(unsigned i=0;i<count[2];++i)if(UInt(bytes.data()+data[2]+i*4)!=4)return false;
        unsigned material=UInt(bytes.data()+cursor);
        if(material>=solidMaterials.size())return false;
        cursor+=12;
        // Validate the complete classic extent/UV tail too, rather than accepting a truncated geoset.
        if(end-cursor<32)return false;
        unsigned extents=UInt(bytes.data()+cursor+28);cursor+=32;
        if(extents>(end-cursor)/28)return false;
        cursor+=size_t(extents)*28;
        if(end-cursor<8 || std::memcmp(bytes.data()+cursor,"UVAS",4))return false;
        unsigned uvSets=UInt(bytes.data()+cursor+4);cursor+=8;
        if(uvSets>32)return false;
        for(unsigned i=0;i<uvSets;++i) {
            size_t uv=0;unsigned uvCount=0;
            if(!Array(bytes,end,cursor,"UVBS",8,uv,uvCount) || uvCount!=count[0])return false;
        }
        if(cursor!=end)return false;
        bool include=solidMaterials[material] && (geosetId>=visible.size() || visible[geosetId]);
        for(unsigned face=0;face<count[4];face+=3) {
            Triangle3 triangle;float average[3]={};
            for(unsigned v=0;v<3;++v) {
                unsigned short index;std::memcpy(&index,bytes.data()+data[4]+(face+v)*2,2);
                if(index>=count[0])return false;
                std::memcpy(triangle.vertices[v],bytes.data()+data[0]+size_t(index)*12,12);
                for(unsigned axis=0;axis<3;++axis) {
                    float value=triangle.vertices[v][axis],normal=Float(bytes.data()+data[1]+size_t(index)*12+axis*4);
                    if(!std::isfinite(value) || !std::isfinite(normal) || std::abs(value)>100000)return false;
                    average[axis]+=normal;
                }
            }
            float a[3],b[3];for(int i=0;i<3;++i){a[i]=triangle.vertices[1][i]-triangle.vertices[0][i];b[i]=triangle.vertices[2][i]-triangle.vertices[0][i];}
            float dot=(a[1]*b[2]-a[2]*b[1])*average[0]+(a[2]*b[0]-a[0]*b[2])*average[1]+(a[0]*b[1]-a[1]*b[0])*average[2];
            if(dot<0)for(int i=0;i<3;++i)std::swap(triangle.vertices[1][i],triangle.vertices[2][i]);
            if(include)candidate.triangles.push_back(triangle);
            if(candidate.triangles.size()>100000)return false;
        }
        at=end;
    }
    if(candidate.triangles.empty())return false;
    result=std::move(candidate);return true;
}
}
