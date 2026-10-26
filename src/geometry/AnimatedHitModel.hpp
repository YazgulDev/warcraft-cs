#pragma once
#include "MdxPose.hpp"
#include "RayBounds.hpp"
#include <cctype>
#include <cstring>
#include <functional>
#include <string>

// Trace authored opaque/cutout triangles in the current skeletal pose, not the empty corners of a model box.
class AnimatedHitModel {
public:
    struct Sequence {unsigned start=0,end=0;};
    struct Contact {float distance=0;bool head=false;};
    bool Load(const std::vector<unsigned char>& bytes);
    bool Ready() const {return !faces_.empty();}
    bool Animated() const {return animated_;}
    size_t TriangleCount() const {return faces_.size();}
    size_t HeadVertices() const {return size_t(std::count_if(vertices_.begin(),vertices_.end(),[](const Vertex& v){return v.head && v.usable;}));}
    const Bounds3& BroadBounds() const {return broad_;}
    const std::vector<Sequence>& Sequences() const {return sequences_;}
    bool Intersect(const float* origin,const float* direction,float limit,int sequence,float time,Contact& contact) const;
private:
    struct Node {unsigned id=0,parent=0xffffffff,flags=0;bool head=false;MdxPose::Track translation,rotation,scale;};
    struct Vertex {std::array<float,3> position;std::vector<unsigned> bones;bool head=false,usable=true;};
    struct Face {unsigned a,b,c,geoset;};
    struct Visibility {float alpha=1;MdxPose::Track track;};
    struct Chunk {size_t begin,end;};
    static unsigned UInt(const unsigned char* at){unsigned value;std::memcpy(&value,at,4);return value;}
    static float Float(const unsigned char* at){float value;std::memcpy(&value,at,4);return value;}
    static bool Array(const std::vector<unsigned char>& bytes,size_t end,size_t& at,const char* tag,size_t stride,size_t& data,unsigned& count);
    static bool Tracks(const std::vector<unsigned char>& bytes,size_t at,size_t end,Node& node);
    std::vector<Node> nodes_;
    std::vector<std::array<float,3>> pivots_;
    std::vector<Vertex> vertices_;
    std::vector<Face> faces_;
    std::vector<Sequence> sequences_;
    std::vector<Visibility> visibility_;
    Bounds3 broad_;
    bool animated_=false;
};

inline bool AnimatedHitModel::Array(const std::vector<unsigned char>& bytes,size_t end,size_t& at,const char* tag,size_t stride,size_t& data,unsigned& count) {
    if(at>end || end-at<8 || std::memcmp(bytes.data()+at,tag,4))return false;
    count=UInt(bytes.data()+at+4);data=at+8;if(count>(end-data)/stride)return false;at=data+size_t(count)*stride;return true;
}
inline bool AnimatedHitModel::Tracks(const std::vector<unsigned char>& bytes,size_t at,size_t end,Node& node) {
    while(at<end) {
        if(end-at<16)return false;auto tag=bytes.data()+at;bool rotation=!std::memcmp(tag,"KGRT",4);
        MdxPose::Track* track=rotation ? &node.rotation : !std::memcmp(tag,"KGTR",4) ? &node.translation : !std::memcmp(tag,"KGSC",4) ? &node.scale : nullptr;
        if(!track || !track->keys.empty())return false;
        unsigned count=UInt(tag+4);track->interpolation=UInt(tag+8);track->global=UInt(tag+12);at+=16;
        unsigned components=rotation ? 4 : 3,values=track->interpolation>1 ? 3 : 1;size_t stride=4+components*4*values;
        if(track->interpolation>3 || count>65536 || count>(end-at)/stride)return false;
        for(unsigned i=0;i<count;++i) {
            MdxPose::Key key;key.time=UInt(bytes.data()+at);at+=4;
            if(!track->keys.empty() && key.time<track->keys.back().time)return false;
            for(unsigned value=0;value<values;++value)for(unsigned component=0;component<components;++component) {
                float f=Float(bytes.data()+at);at+=4;if(!std::isfinite(f) || std::abs(f)>100000)return false;
                (value==0 ? key.value : value==1 ? key.in : key.out)[component]=f;
            }
            track->keys.push_back(key);
        }
    }
    return at==end;
}
inline bool AnimatedHitModel::Load(const std::vector<unsigned char>& bytes) {
    *this=AnimatedHitModel{};
    AnimatedHitModel candidate;std::vector<Chunk> geosets;std::vector<bool> materials;bool version=false;
    if(bytes.size()<4 || std::memcmp(bytes.data(),"MDLX",4))return false;
    for(size_t at=4;at<bytes.size();) {
        if(bytes.size()-at<8)return false;unsigned length=UInt(bytes.data()+at+4);size_t begin=at+8;
        if(length>bytes.size()-begin)return false;size_t end=begin+length;const auto* tag=bytes.data()+at;
        if(!std::memcmp(tag,"VERS",4))version=length==4 && UInt(bytes.data()+begin)==800;
        if(!std::memcmp(tag,"GEOS",4))geosets.push_back({begin,end});
        if(!std::memcmp(tag,"PIVT",4)) {
            if(length%12 || length/12>4096)return false;candidate.pivots_.resize(length/12);
            for(unsigned i=0;i<length/12;++i)for(int axis=0;axis<3;++axis){float f=Float(bytes.data()+begin+i*12+axis*4);
                if(!std::isfinite(f) || std::abs(f)>100000)return false;candidate.pivots_[i][axis]=f;}
        }
        if(!std::memcmp(tag,"SEQS",4)) {
            if(length%132)return false;
            for(size_t p=begin;p<end;p+=132){Sequence s{UInt(bytes.data()+p+80),UInt(bytes.data()+p+84)};if(s.end<s.start)return false;candidate.sequences_.push_back(s);}
        }
        if(!std::memcmp(tag,"BONE",4) || !std::memcmp(tag,"HELP",4)) {
            bool bone=!std::memcmp(tag,"BONE",4);
            for(size_t p=begin;p<end;) {
                if(end-p<96)return false;unsigned size=UInt(bytes.data()+p);
                if(size<96 || size>end-p || (bone && end-p-size<8))return false;
                Node node;node.id=UInt(bytes.data()+p+84);node.parent=UInt(bytes.data()+p+88);node.flags=UInt(bytes.data()+p+92);
                if(node.id>=4096 || (node.parent!=0xffffffff && node.parent>=4096))return false;
                std::string name(reinterpret_cast<const char*>(bytes.data()+p+4),80);name.resize(name.find('\0')==std::string::npos ? name.size() : name.find('\0'));
                for(char& c:name)c=char(std::tolower(static_cast<unsigned char>(c)));
                node.head=name.find("head")!=std::string::npos || name.find("skull")!=std::string::npos;
                if(!Tracks(bytes,p+96,p+size,node))return false;
                if(candidate.nodes_.size()>=4096)return false;
                candidate.nodes_.push_back(std::move(node));p+=size+(bone ? 8 : 0);
            }
        }
        if(!std::memcmp(tag,"MTLS",4))for(size_t p=begin;p<end;) {
            if(end-p<20)return false;unsigned size=UInt(bytes.data()+p);if(size<20 || size>end-p || std::memcmp(bytes.data()+p+12,"LAYS",4))return false;
            unsigned layers=UInt(bytes.data()+p+16);size_t layer=p+20;bool solid=false;
            for(unsigned i=0;i<layers;++i) {
                if(p+size-layer<28)return false;unsigned n=UInt(bytes.data()+layer),filter=UInt(bytes.data()+layer+4);
                if(n<28 || n>p+size-layer || filter>6)return false;
                float alpha=Float(bytes.data()+layer+24);if(!std::isfinite(alpha))return false;
                // Light, glow and particle cards cannot shield a neighboring creature.
                solid=solid || (filter<=1 && alpha>.01f);layer+=n;
            }
            if(layer!=p+size)return false;materials.push_back(solid);p+=size;
        }
        if(!std::memcmp(tag,"GEOA",4))for(size_t p=begin;p<end;) {
            if(end-p<28)return false;unsigned size=UInt(bytes.data()+p),id=UInt(bytes.data()+p+24);
            if(size<28 || size>end-p || id>=4096)return false;
            if(candidate.visibility_.size()<=id)candidate.visibility_.resize(id+1);
            auto& visibility=candidate.visibility_[id];visibility.alpha=Float(bytes.data()+p+4);
            if(!std::isfinite(visibility.alpha))return false;
            for(size_t t=p+28;t<p+size;) {
                if(p+size-t<16)return false;unsigned count=UInt(bytes.data()+t+4),interpolation=UInt(bytes.data()+t+8);
                bool alpha=!std::memcmp(bytes.data()+t,"KGAO",4),color=!std::memcmp(bytes.data()+t,"KGAC",4);
                if((!alpha && !color) || interpolation>3)return false;
                unsigned components=alpha ? 1 : 3,values=interpolation>1 ? 3 : 1;size_t stride=4+components*4*values;
                if(count>(p+size-t-16)/stride)return false;
                if(alpha){visibility.track.interpolation=interpolation;visibility.track.global=UInt(bytes.data()+t+12);
                    if(visibility.track.global!=0xffffffff)return false;}
                t+=16;
                for(unsigned i=0;i<count;++i){MdxPose::Key key;key.time=UInt(bytes.data()+t);t+=4;
                    for(unsigned v=0;v<values;++v)for(unsigned component=0;component<components;++component){float f=Float(bytes.data()+t);t+=4;if(!std::isfinite(f))return false;
                        if(alpha)(v==0 ? key.value : v==1 ? key.in : key.out)[0]=f;}
                    if(alpha){if(!visibility.track.keys.empty() && key.time<visibility.track.keys.back().time)return false;visibility.track.keys.push_back(key);}}
            }
            p+=size;
        }
        at=end;
    }
    if(!version || candidate.sequences_.empty())return false;
    unsigned geosetId=0;
    for(const auto& chunk:geosets)for(size_t at=chunk.begin;at<chunk.end;++geosetId) {
        if(chunk.end-at<4)return false;unsigned size=UInt(bytes.data()+at);if(size<4 || size>chunk.end-at)return false;
        size_t end=at+size,cursor=at+4,data[8];unsigned count[8];
        const char* tags[]={"VRTX","NRMS","PTYP","PCNT","PVTX","GNDX","MTGC","MATS"};const size_t stride[]={12,12,4,4,2,1,4,4};
        for(unsigned i=0;i<8;++i)if(!Array(bytes,end,cursor,tags[i],stride[i],data[i],count[i]))return false;
        if(!count[0] || count[0]>65536 || count[1]!=count[0] || count[5]!=count[0] || count[4]%3 || end-cursor<12)return false;
        for(unsigned i=0;i<count[2];++i)if(UInt(bytes.data()+data[2]+i*4)!=4)return false;
        unsigned material=UInt(bytes.data()+cursor);if(material>=materials.size())return false;
        cursor+=12;
        // Validate the classic geoset extent/UV tail before accepting any vertex/index data.
        if(end-cursor<32)return false;unsigned extents=UInt(bytes.data()+cursor+28);cursor+=32;
        if(extents>(end-cursor)/28)return false;cursor+=size_t(extents)*28;
        if(end-cursor<8 || std::memcmp(bytes.data()+cursor,"UVAS",4))return false;
        unsigned uvSets=UInt(bytes.data()+cursor+4);cursor+=8;if(uvSets>32)return false;
        for(unsigned i=0;i<uvSets;++i){size_t uv=0;unsigned n=0;if(!Array(bytes,end,cursor,"UVBS",8,uv,n) || n!=count[0])return false;}
        if(cursor!=end)return false;
        if(!materials[material]){at=end;continue;}
        if(candidate.vertices_.size()+count[0]>262144 || candidate.faces_.size()+count[4]/3>200000)return false;
        std::vector<std::vector<unsigned>> groups;unsigned offset=0;
        for(unsigned i=0;i<count[6];++i) {
            unsigned n=UInt(bytes.data()+data[6]+i*4);if(!n || n>32 || n>count[7]-offset)return false;
            std::vector<unsigned> group;for(unsigned j=0;j<n;++j)group.push_back(UInt(bytes.data()+data[7]+(offset+j)*4));
            groups.push_back(std::move(group));offset+=n;
        }
        if(offset!=count[7])return false;unsigned base=unsigned(candidate.vertices_.size());
        for(unsigned i=0;i<count[0];++i) {
            unsigned group=bytes[data[5]+i];if(group>=groups.size())return false;Vertex vertex;vertex.bones=groups[group];
            for(int axis=0;axis<3;++axis){float f=Float(bytes.data()+data[0]+i*12+axis*4);if(!std::isfinite(f) || std::abs(f)>100000)return false;vertex.position[axis]=f;}
            candidate.vertices_.push_back(std::move(vertex));
        }
        for(unsigned i=0;i<count[4];i+=3){unsigned short index[3];std::memcpy(index,bytes.data()+data[4]+i*2,6);
            for(auto v:index)if(v>=count[0])return false;candidate.faces_.push_back({base+index[0],base+index[1],base+index[2],geosetId});}
        at=end;
    }
    if(candidate.faces_.empty())return false;
    std::vector<int> ids(4096,-1);
    for(unsigned i=0;i<candidate.nodes_.size();++i){auto id=candidate.nodes_[i].id;if(ids[id]>=0 || id>=candidate.pivots_.size())return false;ids[id]=int(i);}
    for(auto& vertex:candidate.vertices_) {
        unsigned head=0;
        for(unsigned& bone:vertex.bones) {
            if(bone>=ids.size() || ids[bone]<0)return false;bone=unsigned(ids[bone]);
            unsigned current=bone,depth=0;bool isHead=false;
            for(;;) {
                if(++depth>candidate.nodes_.size())return false;const auto& node=candidate.nodes_[current];isHead|=node.head;
                // Billboard/light cards and unverified global/inheritance effects are decorative,
                // not a reason to replace the whole creature with its oversized broad box.
                if((node.flags&0xFB) || node.translation.global!=0xffffffff || node.rotation.global!=0xffffffff || node.scale.global!=0xffffffff)vertex.usable=false;
                candidate.animated_|=!node.translation.keys.empty() || !node.rotation.keys.empty() || !node.scale.keys.empty();
                if(node.parent==0xffffffff)break;if(ids[node.parent]<0)return false;current=unsigned(ids[node.parent]);
            }
            if(isHead)++head;
        }
        vertex.head=head*2>=vertex.bones.size();
    }
    candidate.faces_.erase(std::remove_if(candidate.faces_.begin(),candidate.faces_.end(),[&](const Face& f){
        return !candidate.vertices_[f.a].usable || !candidate.vertices_[f.b].usable || !candidate.vertices_[f.c].usable;
    }),candidate.faces_.end());
    if(candidate.faces_.empty())return false;
    for(const auto& v:candidate.visibility_)candidate.animated_|=!v.track.keys.empty();
    // Broad bounds include every authored sequence; they only reject work, never decide a hit or headshot.
    bool bounds=false;for(size_t at=4;at+8<=bytes.size();) {
        unsigned length=UInt(bytes.data()+at+4);size_t begin=at+8,end=begin+length;
        auto include=[&](size_t p){
            // Empty decay sequences use inverted FLT_MAX bounds; they must not invalidate a live mesh.
            for(int axis=0;axis<3;++axis){float lo=Float(bytes.data()+p+axis*4),hi=Float(bytes.data()+p+12+axis*4);
                if(!std::isfinite(lo) || !std::isfinite(hi) || lo>hi || hi-lo>100000)return false;}
            for(int axis=0;axis<3;++axis){float lo=Float(bytes.data()+p+axis*4),hi=Float(bytes.data()+p+12+axis*4);
            if(!bounds){candidate.broad_.minimum[axis]=lo;candidate.broad_.maximum[axis]=hi;}
            else{candidate.broad_.minimum[axis]=std::min(candidate.broad_.minimum[axis],lo);candidate.broad_.maximum[axis]=std::max(candidate.broad_.maximum[axis],hi);}}
            bounds=true;return true;};
        if(!std::memcmp(bytes.data()+at,"MODL",4) && length>=372)include(begin+344);
        if(!std::memcmp(bytes.data()+at,"SEQS",4))for(size_t p=begin;p+132<=end;p+=132)include(p+108);
        at=end;
    }
    if(!bounds)return false;*this=std::move(candidate);return true;
}
inline bool AnimatedHitModel::Intersect(const float* origin,const float* direction,float limit,int sequence,float time,Contact& contact) const {
    if(!Ready() || sequence<0 || size_t(sequence)>=sequences_.size() || !std::isfinite(time))return false;
    float broadEntry=0;if(!IntersectBounds(broad_,origin,direction,limit,broadEntry))return false;
    const auto& range=sequences_[sequence];time=std::clamp(time,float(range.start),float(range.end));
    std::vector<MdxPose::Matrix> pose(nodes_.size());std::vector<unsigned char> done(nodes_.size());
    std::vector<int> ids(pivots_.size(),-1);for(unsigned i=0;i<nodes_.size();++i)ids[nodes_[i].id]=int(i);
    std::function<void(unsigned)> evaluate=[&](unsigned index) {
        if(done[index])return;const auto& n=nodes_[index];
        auto matrix=MdxPose::Transform(pivots_[n.id].data(),n.translation.Sample(time,range.start,range.end,{0,0,0,0}),
            n.rotation.Sample(time,range.start,range.end,{0,0,0,1},true),n.scale.Sample(time,range.start,range.end,{1,1,1,0}));
        if(n.parent!=0xffffffff){unsigned parent=unsigned(ids[n.parent]);evaluate(parent);auto inherited=pose[parent];
            // Classic cloth/shield nodes can explicitly inherit rotation/translation without parent scale.
            if(n.flags&4)for(int c=0;c<3;++c){float length=0;for(int r=0;r<3;++r)length+=inherited[r*4+c]*inherited[r*4+c];
                if(length>.000001f)for(int r=0;r<3;++r)inherited[r*4+c]/=std::sqrt(length);}
            matrix=MdxPose::Multiply(inherited,matrix);}
        pose[index]=matrix;done[index]=1;
    };
    std::vector<std::array<float,3>> positions(vertices_.size());
    for(unsigned i=0;i<vertices_.size();++i){const auto& vertex=vertices_[i];if(!vertex.usable)continue;auto& position=positions[i];
        for(auto bone:vertex.bones){evaluate(bone);auto p=MdxPose::Point(pose[bone],vertex.position.data());for(int axis=0;axis<3;++axis)position[axis]+=p[axis]/vertex.bones.size();}}
    bool hit=false;float nearest=limit;
    for(const auto& face:faces_) {float distance=0;
        // Alternate weapon/carry/death geosets use native sequence opacity; hidden meshes cannot intercept a ray.
        if(face.geoset<visibility_.size()){const auto& v=visibility_[face.geoset];
            if(v.track.Sample(time,range.start,range.end,{v.alpha,0,0,0})[0]<=.01f)continue;}
        if(MdxPose::Triangle(positions[face.a].data(),positions[face.b].data(),positions[face.c].data(),origin,direction,nearest,distance)) {
            nearest=distance;hit=true;contact={distance,(int(vertices_[face.a].head)+int(vertices_[face.b].head)+int(vertices_[face.c].head))>=2};
        }
    }
    return hit;
}
