#include "CollisionMeshFixture.hpp"
#include "../src/geometry/StaticModelMesh.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>

namespace {
using Bytes=std::vector<unsigned char>;
void UInt(Bytes& bytes,unsigned value){auto* data=reinterpret_cast<unsigned char*>(&value);bytes.insert(bytes.end(),data,data+4);}
void Float(Bytes& bytes,float value){auto* data=reinterpret_cast<unsigned char*>(&value);bytes.insert(bytes.end(),data,data+4);}
void Chunk(Bytes& bytes,const char* tag,const Bytes& body){bytes.insert(bytes.end(),tag,tag+4);UInt(bytes,unsigned(body.size()));bytes.insert(bytes.end(),body.begin(),body.end());}
Bytes Model(const CylinderMesh& mesh) {
    Bytes bytes={'M','D','L','X'},body;UInt(body,800);Chunk(bytes,"VERS",body);
    body.assign(132,0);std::memcpy(body.data(),"Stand",5);unsigned end=1000;std::memcpy(body.data()+84,&end,4);Chunk(bytes,"SEQS",body);
    Bytes layer;UInt(layer,28);for(int i=0;i<5;++i)UInt(layer,0);Float(layer,1);
    body.clear();UInt(body,48);UInt(body,0);UInt(body,0);body.insert(body.end(),{'L','A','Y','S'});UInt(body,1);body.insert(body.end(),layer.begin(),layer.end());Chunk(bytes,"MTLS",body);
    Bytes vertices,normals,indices;
    for(const auto& triangle:mesh.triangles) {
        float a[3],b[3];for(int i=0;i<3;++i){a[i]=triangle.vertices[1][i]-triangle.vertices[0][i];b[i]=triangle.vertices[2][i]-triangle.vertices[0][i];}
        float n[]={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
        float length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
        for(const auto& point:triangle.vertices) {
            unsigned short index=static_cast<unsigned short>(vertices.size()/12);
            indices.push_back(static_cast<unsigned char>(index));indices.push_back(static_cast<unsigned char>(index>>8));
            for(int i=0;i<3;++i){Float(vertices,point[i]);Float(normals,n[i]/length);}
        }
    }
    Bytes geo(4,0);
    auto array=[&](const char* tag,unsigned count,const Bytes& data) {
        geo.insert(geo.end(),tag,tag+4);UInt(geo,count);geo.insert(geo.end(),data.begin(),data.end());
    };
    unsigned count=unsigned(vertices.size()/12);
    array("VRTX",count,vertices);array("NRMS",count,normals);
    body.clear();UInt(body,4);array("PTYP",1,body);body.clear();UInt(body,count);array("PCNT",1,body);
    array("PVTX",count,indices);array("GNDX",count,Bytes(count,0));
    body.clear();UInt(body,1);array("MTGC",1,body);body.clear();UInt(body,0);array("MATS",1,body);
    for(int i=0;i<3;++i)UInt(geo,0);
    geo.resize(geo.size()+28,0);UInt(geo,0);geo.insert(geo.end(),{'U','V','A','S'});UInt(geo,1);
    array("UVBS",count,Bytes(count*8,0));unsigned size=unsigned(geo.size());std::memcpy(geo.data(),&size,4);
    Chunk(bytes,"GEOS",geo);return bytes;
}
}
int main(int argc,char** argv) {
    CylinderMesh arch;AddMeshBox(arch,{{-10,-80,0},{10,-20,200}});AddMeshBox(arch,{{-10,20,0},{10,80,200}});
    AddMeshBox(arch,{{-10,-20,140},{10,20,200}});
    assert(!arch.Contact(0,0,16,0,108));assert(arch.Contact(0,0,24,0,108));
    assert(arch.Contact(0,0,16,0,150));assert(!arch.Contact(0,0,16,0,66));
    float height=0;assert(!arch.Floor(0,0,16,27,.69f,height));
    assert(arch.Ceiling(0,0,16,0,height) && height==140);
    CylinderMesh plinth;AddMeshBox(plinth,{{-80,-80,0},{80,80,40}});AddMeshBox(plinth,{{-20,50,40},{20,80,300}});
    assert(plinth.Floor(0,0,24,60,.69f,height) && height==40);
    assert(!plinth.Contact(0,0,24,40,148));assert(plinth.Contact(0,60,24,40,148));
    ModelTransform pose;float angle=.63f;pose.matrix[0]=2*std::cos(angle);pose.matrix[1]=-2*std::sin(angle);
    pose.matrix[3]=2*std::sin(angle);pose.matrix[4]=2*std::cos(angle);pose.matrix[8]=2;pose.position[0]=300;pose.position[2]=100;
    auto rotated=arch.Transformed(pose);
    assert(!rotated.Contact(300,0,32,100,316));assert(rotated.Contact(300,0,48,100,316));
    assert(rotated.Ceiling(300,0,32,100,height) && std::abs(height-380)<.001f);
    CylinderMesh wall;AddMeshBox(wall,{{-10,-100,0},{10,100,200}});float outward[2]={};
    assert(wall.Contact(0,0,16,0,108,outward) && outward[0]<-.99f);
    CylinderMesh ramp;ramp.triangles={{{{-100,-100,0},{100,-100,400},{100,100,400}}},
        {{{-100,-100,0},{100,100,400},{-100,100,0}}}};
    float slopeNormal[3]={};
    assert(ramp.Floor(0,0,24,500,.0001f,height,slopeNormal));
    assert(std::abs(height-248)<.01f && slopeNormal[2]<.46f && slopeNormal[0]<-.89f);
    // The uphill end may exceed the old feet while a lower part still crosses the swept foot.
    assert(ramp.Floor(0,0,24,220,.0001f,height,slopeNormal) && std::abs(height-220)<.01f);
    pose.matrix[8]=-2;pose.position[2]=500;auto mirrored=arch.Transformed(pose);
    assert(mirrored.Floor(300,0,32,500,.69f,height) && std::abs(height-220)<.001f);
    auto bytes=Model(arch);CylinderMesh decoded;
    assert(StaticModelMesh::Load(bytes,decoded) && decoded.triangles.size()==arch.triangles.size());
    assert(!decoded.Contact(0,0,16,0,108) && decoded.Contact(0,0,24,0,108));
    // Malformed arrays cannot publish a partial mesh or permission to ignore native blockers.
    auto bad=bytes;bad.resize(bad.size()-3);assert(!StaticModelMesh::Load(bad,decoded) && decoded.triangles.empty());
    bad=bytes;bad[12]=0;assert(!StaticModelMesh::Load(bad,decoded));
    // Only stationary Stand geometry is eligible; animated Stand poses keep conservative bounds.
    auto animated=[&](unsigned time,float translation) {
        Bytes tracks;tracks.insert(tracks.end(),{'K','G','T','R'});UInt(tracks,1);UInt(tracks,1);UInt(tracks,0xffffffff);
        UInt(tracks,time);Float(tracks,translation);Float(tracks,0);Float(tracks,0);
        Bytes node(96,0);unsigned size=96+unsigned(tracks.size());std::memcpy(node.data(),&size,4);
        node.insert(node.end(),tracks.begin(),tracks.end());UInt(node,0xffffffff);UInt(node,0xffffffff);
        auto model=bytes;Chunk(model,"BONE",node);return model;
    };
    assert(!StaticModelMesh::Load(animated(0,10),decoded));
    assert(StaticModelMesh::Load(animated(0,0),decoded));
    assert(StaticModelMesh::Load(animated(2500,10),decoded));
    // Optional private verification reads the owner's MDX; the repository contains only synthetic fixtures.
    for(int i=1;i<argc;++i) {
        std::ifstream file(argv[i],std::ios::binary);Bytes owned((std::istreambuf_iterator<char>(file)),{});
        assert(!owned.empty());bool loaded=StaticModelMesh::Load(owned,decoded);
        std::printf("Owned static model %s: %s triangles=%zu\n",argv[i],loaded ? "decoded" : "conservative fallback",decoded.triangles.size());
        if(i==1)assert(loaded);
    }
    std::puts("PASS circular mesh fit, actual-height support/ceilings, transformed geometry and bounded MDX decoding.");
}
