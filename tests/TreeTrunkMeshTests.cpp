#include "../src/TreeTrunkMesh.hpp"
#include "../src/ModelTransform.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>

namespace {
void Require(bool condition,const char* message) {
    if (!condition) { std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1); }
}
template<class T> void Append(std::vector<unsigned char>& bytes,const T& value) {
    auto data=reinterpret_cast<const unsigned char*>(&value);bytes.insert(bytes.end(),data,data+sizeof(value));
}
void Tag(std::vector<unsigned char>& bytes,const char* tag) { bytes.insert(bytes.end(),tag,tag+4); }
std::vector<unsigned char> Fixture() {
    // Original synthetic geometry: a trunk, detached canopy, and a wide short death-only stump.
    std::vector<std::array<float,3>> vertices;
    auto box=[&](float radius,float height) {
        for (float z:{0.f,height}) for (const auto& xy:std::vector<std::array<float,2>>{{-radius,-radius},{radius,-radius},{radius,radius},{-radius,radius}})
            vertices.push_back({xy[0],xy[1],z});
    };
    box(10,200);box(60,18);
    vertices.insert(vertices.end(),{{-100,-100,90},{100,-100,90},{100,100,90},{-100,100,90}});
    const unsigned short cube[]={0,1,5,0,5,4,1,2,6,1,6,5,2,3,7,2,7,6,3,0,4,3,4,7,0,3,2,0,2,1,4,5,6,4,6,7};
    std::vector<unsigned short> indices(std::begin(cube),std::end(cube));
    for (unsigned short index:cube) indices.push_back(index+8);
    indices.insert(indices.end(),{16,17,18,16,18,19});
    std::vector<unsigned char> geoset;Append(geoset,0U);
    Tag(geoset,"VRTX");Append(geoset,unsigned(vertices.size()));for (const auto& point:vertices) for (float v:point) Append(geoset,v);
    Tag(geoset,"NRMS");Append(geoset,unsigned(vertices.size()));for (size_t i=0;i<vertices.size()*3;++i) Append(geoset,0.f);
    Tag(geoset,"PTYP");Append(geoset,1U);Append(geoset,4U);
    Tag(geoset,"PCNT");Append(geoset,1U);Append(geoset,unsigned(indices.size()));
    Tag(geoset,"PVTX");Append(geoset,unsigned(indices.size()));for (unsigned short index:indices) Append(geoset,index);
    unsigned length=unsigned(geoset.size());std::memcpy(geoset.data(),&length,4);
    std::vector<unsigned char> bytes;Tag(bytes,"MDLX");Tag(bytes,"SEQS");Append(bytes,132U);
    size_t seq=bytes.size();bytes.resize(seq+132);std::memcpy(bytes.data()+seq,"Spell EatTree",13);
    Tag(bytes,"GEOS");Append(bytes,length);bytes.insert(bytes.end(),geoset.begin(),geoset.end());return bytes;
}
}
int main(int argc,char** argv) {
    Bounds3 standing={{-100,-100,0},{100,100,240}};
    auto bytes=Fixture();TreeTrunkMesh trunk;trunk.Load("Tree.mdx",bytes,standing);
    Require(trunk.IsTree() && trunk.TriangleCount()==12,"Only the tall root-connected trunk must remain");
    float origin[]={-300,60,96},direction[]={1,0,0},entry=0;
    Require(IntersectBounds(standing,origin,direction,500,entry),"Original canopy box must reproduce the false blocker");
    Require(!trunk.Intersect(origin,direction,500,entry),"Enemy beside the trunk must not be blocked by the canopy box");
    origin[1]=0;Require(trunk.Intersect(origin,direction,500,entry) && std::abs(entry-290)<.001f,"Direct trunk shots must still hit the front surface");
    Require(!trunk.Intersect(origin,direction,289,entry),"Tree beyond a nearer enemy must not steal its hit");
    origin[0]=300;direction[0]=-1;
    Require(trunk.Intersect(origin,direction,500,entry) && std::abs(entry-290)<.001f,"Back-facing trunk triangles must also block");
    origin[0]=-300;origin[1]=40;origin[2]=10;direction[0]=1;
    Require(!trunk.Intersect(origin,direction,500,entry),"Invisible death-only stump must not expand the standing tree");
    // Full sprite transforms must retain world-distance ordering, not normalize the local direction.
    ModelTransform transform;transform.matrix[0]=transform.matrix[4]=2;transform.matrix[8]=3;
    transform.position[0]=400;transform.position[1]=100;transform.position[2]=64;
    float world[]={100,100,352},localOrigin[3],localDirection[3];
    Require(transform.LocalRay(world,direction,localOrigin,localDirection) &&
        trunk.Intersect(localOrigin,localDirection,400,entry) && std::abs(entry-280)<.001f,"Scaled native trees must preserve world hit distance");
    TreeTrunkMesh renamed;renamed.Load("Imported.mdx",bytes,standing);
    Require(renamed.IsTree() && renamed.TriangleCount()==12,"EatTree animation identifies renamed tree imports");
    auto corrupt=bytes;corrupt.resize(corrupt.size()-1);trunk.Load("Tree.mdx",corrupt,standing);
    Require(trunk.IsTree() && !trunk.Intersect(localOrigin,localDirection,500,entry),"Malformed tree must never fall back to a canopy box");
    // Optional owner-only assets verify the parser against real files without committing their contents.
    for (int i=1;i<argc;++i) {
        std::ifstream file(argv[i],std::ios::binary);std::vector<unsigned char> owned((std::istreambuf_iterator<char>(file)),{});
        bool bounds=false;
        for (size_t at=4;at+8<=owned.size();) {
            unsigned n=0;std::memcpy(&n,owned.data()+at+4,4);size_t body=at+8;
            if (n>owned.size()-body) break;
            if (!std::memcmp(owned.data()+at,"MODL",4) && n>=372) { std::memcpy(&standing,owned.data()+body+344,sizeof(standing));bounds=true; }
            at=body+n;
        }
        trunk.Load(argv[i],owned,standing);
        Require(bounds && trunk.IsTree() && trunk.TriangleCount()>0,"Owned stock tree must have decoded trunk geometry");
        std::printf("PASS owned tree trunk geometry: %u triangles\n",unsigned(trunk.TriangleCount()));
    }
    std::puts("PASS tree canopy/stump exclusion, direct trunk hits, nearest-target ordering, double-sided picking, scale and corrupt model rejection");
}
