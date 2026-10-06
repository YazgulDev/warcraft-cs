#include "TreeTrunkMesh.hpp"
#include <array>
#include <cctype>
#include <cstring>
#include <limits>
#include <numeric>

namespace {
unsigned UInt(const unsigned char* bytes) { unsigned value=0;std::memcpy(&value,bytes,4);return value; }
bool Array(const std::vector<unsigned char>& bytes,size_t end,size_t& cursor,const char* tag,
    size_t stride,size_t& data,unsigned& count) {
    if (cursor>end || end-cursor<8 || std::memcmp(bytes.data()+cursor,tag,4)) return false;
    count=UInt(bytes.data()+cursor+4);data=cursor+8;
    // Every array is bounded by its inclusive geoset size, including malformed counts and truncation.
    if (count>(end-data)/stride) return false;
    cursor=data+size_t(count)*stride;return true;
}
unsigned Root(std::vector<unsigned>& parents,unsigned vertex) {
    while (parents[vertex]!=vertex) { parents[vertex]=parents[parents[vertex]];vertex=parents[vertex]; }
    return vertex;
}
bool Geoset(const std::vector<unsigned char>& bytes,size_t start,size_t end,const Bounds3& standing,
    std::vector<Triangle3>& output) {
    size_t cursor=start+4,vertices=0,normals=0,types=0,groups=0,faces=0;
    unsigned count=0,normalCount=0,typeCount=0,groupCount=0,faceCount=0;
    if (!Array(bytes,end,cursor,"VRTX",12,vertices,count) || !count || count>65536 ||
        !Array(bytes,end,cursor,"NRMS",12,normals,normalCount) ||
        !Array(bytes,end,cursor,"PTYP",4,types,typeCount) ||
        !Array(bytes,end,cursor,"PCNT",4,groups,groupCount) ||
        !Array(bytes,end,cursor,"PVTX",2,faces,faceCount) || faceCount%3) return false;
    for (unsigned i=0;i<typeCount;++i) if (UInt(bytes.data()+types+size_t(i)*4)!=4) return false;
    std::vector<std::array<float,3>> points(count);
    std::vector<unsigned> parents(count);std::iota(parents.begin(),parents.end(),0U);
    for (unsigned i=0;i<count;++i) {
        std::memcpy(points[i].data(),bytes.data()+vertices+size_t(i)*12,12);
        for (float value:points[i]) if (!std::isfinite(value)) return false;
    }
    std::vector<unsigned short> indices(faceCount);
    std::memcpy(indices.data(),bytes.data()+faces,size_t(faceCount)*2);
    for (unsigned i=0;i<faceCount;i+=3) {
        for (unsigned j=0;j<3;++j) if (indices[i+j]>=count) return false;
        for (unsigned j=1;j<3;++j) parents[Root(parents,indices[i+j])]=Root(parents,indices[i]);
    }
    std::vector<float> low(count,std::numeric_limits<float>::infinity()),high(count,-std::numeric_limits<float>::infinity());
    for (unsigned i=0;i<count;++i) {
        unsigned root=Root(parents,i);low[root]=std::min(low[root],points[i][2]);high[root]=std::max(high[root],points[i][2]);
    }
    float height=standing.maximum[2]-standing.minimum[2];
    float groundTolerance=std::max(2.f,height*.01f),minimumHeight=std::max(12.f,height*.15f);
    for (unsigned i=0;i<faceCount;i+=3) {
        unsigned root=Root(parents,indices[i]);
        // Canopy cards are disconnected from the root; short stump/debris meshes are visible only on death.
        if (low[root]>standing.minimum[2]+groundTolerance || high[root]-low[root]<minimumHeight) continue;
        Triangle3 triangle;
        for (unsigned j=0;j<3;++j) std::copy(points[indices[i+j]].begin(),points[indices[i+j]].end(),triangle.vertices[j]);
        output.push_back(triangle);
    }
    return true;
}
bool EatTreeSequence(const std::vector<unsigned char>& bytes,size_t start,size_t length) {
    for (size_t sequence=start;sequence+132<=start+length;sequence+=132) {
        std::string name(reinterpret_cast<const char*>(bytes.data()+sequence),80);
        for (char& c:name) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (name.find("eattree")!=std::string::npos) return true;
    }
    return false;
}
}
void TreeTrunkMesh::Load(const std::string& path,const std::vector<unsigned char>& bytes,const Bounds3& standing) {
    triangles_.clear();std::string name=path;
    for (char& c:name) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    tree_=name.find("tree")!=std::string::npos;
    if (bytes.size()<4 || std::memcmp(bytes.data(),"MDLX",4)) return;
    std::vector<std::pair<size_t,size_t>> geosets;
    for (size_t at=4;at+8<=bytes.size();) {
        unsigned length=UInt(bytes.data()+at+4);size_t body=at+8;
        if (length>bytes.size()-body) return;
        if (!std::memcmp(bytes.data()+at,"SEQS",4) && EatTreeSequence(bytes,body,length)) tree_=true;
        if (!std::memcmp(bytes.data()+at,"GEOS",4)) geosets.emplace_back(body,body+length);
        at=body+length;
    }
    if (!tree_) return;
    std::vector<Triangle3> candidate;
    for (const auto& chunk:geosets) for (size_t at=chunk.first;at<chunk.second;) {
        if (chunk.second-at<4) return;
        unsigned length=UInt(bytes.data()+at);
        if (length<12 || length>chunk.second-at || !Geoset(bytes,at,at+length,standing,candidate)) return;
        at+=length;
    }
    // A malformed tree never falls back to the original canopy-sized box.
    triangles_=std::move(candidate);
}
bool TreeTrunkMesh::Intersect(const float* origin,const float* direction,float limit,float& entry) const {
    bool hit=false;
    for (const Triangle3& triangle:triangles_) {
        float distance=0;
        if (IntersectTriangle(triangle,origin,direction,limit,distance)) { limit=distance;entry=distance;hit=true; }
    }
    return hit;
}
