#pragma once
#include "../src/geometry/RayBounds.hpp"
#include <cstring>
#include <vector>

// Project-authored two-part creature: a body triangle and an independently animated head triangle.
namespace MdxHitFixture {
using Bytes=std::vector<unsigned char>;
inline void UInt(Bytes& b,unsigned v){auto p=reinterpret_cast<unsigned char*>(&v);b.insert(b.end(),p,p+4);}
inline void Real(Bytes& b,float v){auto p=reinterpret_cast<unsigned char*>(&v);b.insert(b.end(),p,p+4);}
inline void Tag(Bytes& b,const char* tag){b.insert(b.end(),tag,tag+4);}
inline void Set(Bytes& b,size_t at,unsigned v){std::memcpy(b.data()+at,&v,4);}
inline void Chunk(Bytes& b,const char* tag,const Bytes& value){Tag(b,tag);UInt(b,unsigned(value.size()));b.insert(b.end(),value.begin(),value.end());}
inline Bytes Node(const char* name,unsigned id,unsigned parent,bool animate,unsigned flags=256) {
    Bytes node(96);std::memcpy(node.data()+4,name,std::strlen(name));Set(node,84,id);Set(node,88,parent);Set(node,92,flags);
    if(animate){Tag(node,"KGTR");UInt(node,2);UInt(node,1);UInt(node,0xffffffff);
        for(unsigned time:{0u,1000u}){UInt(node,time);Real(node,time ? 20.f : 0.f);Real(node,0);Real(node,0);}}
    Set(node,0,unsigned(node.size()));UInt(node,0xffffffff);UInt(node,0xffffffff);return node;
}
inline Bytes Geoset(bool head) {
    Bytes g(4),vertices,normals,indices,groups,types,counts,bones,groupCounts;
    const float body[3][3]={{0,-20,0},{0,20,0},{0,0,100}},skull[3][3]={{0,-10,100},{0,10,100},{0,0,140}};
    for(int i=0;i<3;++i)for(int j=0;j<3;++j){Real(vertices,head ? skull[i][j] : body[i][j]);Real(normals,j==0 ? 1.f : 0.f);}
    UInt(types,4);UInt(counts,3);indices={0,0,1,0,2,0};groups={0,0,0};UInt(groupCounts,1);UInt(bones,head ? 1 : 0);
    auto array=[&](const char* tag,const Bytes& values,unsigned count){Tag(g,tag);UInt(g,count);g.insert(g.end(),values.begin(),values.end());};
    array("VRTX",vertices,3);array("NRMS",normals,3);array("PTYP",types,1);array("PCNT",counts,1);array("PVTX",indices,3);
    array("GNDX",groups,3);array("MTGC",groupCounts,1);array("MATS",bones,1);
    UInt(g,0);UInt(g,0);UInt(g,0);for(int i=0;i<7;++i)Real(g,0);UInt(g,0);
    Tag(g,"UVAS");UInt(g,1);Bytes uv(24);array("UVBS",uv,3);Set(g,0,unsigned(g.size()));return g;
}
inline Bytes Model(bool hiddenHead=false,unsigned headFlags=256) {
    Bytes result;Tag(result,"MDLX");Bytes version;UInt(version,800);Chunk(result,"VERS",version);
    Bytes model(372);Bounds3 broad={{-100,-100,-10},{100,100,200}};std::memcpy(model.data()+344,&broad,24);Chunk(result,"MODL",model);
    Bytes seq(132);std::memcpy(seq.data(),"Stand",5);Set(seq,80,0);Set(seq,84,1000);std::memcpy(seq.data()+108,&broad,24);Chunk(result,"SEQS",seq);
    Bytes material(20);std::memcpy(material.data()+12,"LAYS",4);Set(material,16,1);
    Bytes layer(28);Set(layer,0,28);float alpha=1;std::memcpy(layer.data()+24,&alpha,4);material.insert(material.end(),layer.begin(),layer.end());Set(material,0,unsigned(material.size()));Chunk(result,"MTLS",material);
    Bytes nodes=Node("Body",0,0xffffffff,false),head=Node("Bone_Head",1,0,true,headFlags);nodes.insert(nodes.end(),head.begin(),head.end());Chunk(result,"BONE",nodes);
    Bytes pivots(24);Chunk(result,"PIVT",pivots);Bytes geo=Geoset(false),headGeo=Geoset(true);geo.insert(geo.end(),headGeo.begin(),headGeo.end());Chunk(result,"GEOS",geo);
    if(hiddenHead){Bytes visibility(28);Set(visibility,0,28);Set(visibility,24,1);Chunk(result,"GEOA",visibility);}
    return result;
}
}
