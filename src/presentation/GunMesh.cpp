#include "GunMesh.hpp"
#include "WeaponAnimation.hpp"
#include "MuzzleFlashView.hpp"
#include "../platform/WarcraftApi.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

template<class T> static void Read(std::ifstream& file, T& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!file) throw std::runtime_error("truncated mesh cache");
}
bool GunMesh::Load(const std::string& path) {
    Reset(true);
    // Generated cache counts are bounded before allocation or GPU upload.
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        unsigned magic, textures, meshes, sequences;
        Read(file, magic); Read(file, textures); Read(file, meshes); Read(file, sequences); Read(file, boneCount_);
        if (magic != 0x32474357 || textures > 128 || meshes > 256 || sequences > 2048 || !boneCount_ || boneCount_ > 128)
            throw std::runtime_error("invalid mesh cache header");
        for (unsigned i = 0; i < textures; ++i) {
            unsigned width, height; Read(file, width); Read(file, height);
            if (!width || !height || width > 2048 || height > 2048) throw std::runtime_error("invalid skin size");
            std::vector<unsigned char> pixels(width * height * 4);
            file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
            if (!file) throw std::runtime_error("truncated skin");
            GLuint texture; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // Skins are UV atlases; clamp the edges to avoid sampling the opposite atlas border.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
            // Retail RGBA data is tightly packed; Warcraft's pixel upload stride must not leak in.
            glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0); glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            glPopClientAttrib();
            // Mipmaps reduce shimmering on small slide/hand details without replacing the original skins.
            using GenerateMipmap = void (APIENTRY*)(GLenum);
            auto generateMipmap = reinterpret_cast<GenerateMipmap>(wglGetProcAddress("glGenerateMipmap"));
            if (generateMipmap) { generateMipmap(GL_TEXTURE_2D); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); }
            textures_.push_back(texture);
        }
        for (unsigned i = 0; i < meshes; ++i) {
            Mesh mesh; unsigned count; Read(file, mesh.texture); Read(file, count);
            if (mesh.texture >= textures || count > 100000 || count % 3) throw std::runtime_error("invalid triangle list");
            mesh.vertices.resize(count); file.read(reinterpret_cast<char*>(mesh.vertices.data()), count * sizeof(Vertex));
            for (const auto& v : mesh.vertices) if (v.bone >= boneCount_) throw std::runtime_error("invalid vertex bone");
            meshes_.push_back(std::move(mesh));
        }
        for (unsigned i = 0; i < sequences; ++i) {
            char label[32]; file.read(label, 32); label[31] = 0;
            Sequence sequence; sequence.name = label; Read(file, sequence.fps); Read(file, sequence.frames);
            if (!sequence.frames || sequence.frames > 10000 || sequence.fps <= 0 || sequence.fps > 1000)
                throw std::runtime_error("invalid animation");
            sequence.bones.resize(sequence.frames * boneCount_ * 12);
            file.read(reinterpret_cast<char*>(sequence.bones.data()), sequence.bones.size() * sizeof(float));
            if (!file) throw std::runtime_error("truncated animation");
            sequences_.push_back(std::move(sequence));
        }
        // New private caches carry animated barrel anchors; old WCG2 caches remain supported.
        if(file.peek()!=std::char_traits<char>::eof()) {
            unsigned footer=0;Read(file,footer);
            if(footer==0x314D4357) {
                unsigned count=0;Read(file,count);if(count>8)throw std::runtime_error("unsafe muzzle count");
                for(unsigned i=0;i<count;++i) {
                    Muzzle muzzle;Read(file,muzzle.bone);Read(file,muzzle.x);Read(file,muzzle.y);Read(file,muzzle.z);
                    if(muzzle.bone>=boneCount_ || !std::isfinite(muzzle.x) || !std::isfinite(muzzle.y) || !std::isfinite(muzzle.z))throw std::runtime_error("invalid muzzle anchor");
                    muzzles_.push_back(muzzle);
                }
            }
        }
        bool authored=!muzzles_.empty();
        if(!authored && !sequences_.empty()) {
            // Fit legacy/procedural caches from their actual forward-most idle vertex, never a per-gun screen coordinate.
            const Sequence* idle=&sequences_.front();
            for(const auto& item:sequences_)if(item.name.find("idle")==0){idle=&item;break;}
            float forward=-1e30f;Muzzle muzzle={};
            for(const auto& mesh:meshes_)for(const auto& v:mesh.vertices) {
                const float* m=&idle->bones[v.bone*12];float x=m[0]*v.x+m[1]*v.y+m[2]*v.z+m[3];
                if(x>forward){forward=x;muzzle={v.bone,v.x,v.y,v.z};}
            }
            if(forward>-1e29f)muzzles_.push_back(muzzle);
        }
        label_=path.substr(path.find_last_of("/\\")+1);
        wc3::Log("viewmodel loaded %s: %u skins, %u meshes, %u sequences; muzzle=%s count=%u",path.c_str(),textures,meshes,sequences,authored ? "attachment" : "geometry",unsigned(muzzles_.size()));
        return true;
    } catch (const std::exception& e) { wc3::Log("viewmodel failed: %s", e.what()); return false; }
}
void GunMesh::Reset(bool deleteTextures) {
    // Deleted contexts own no valid names; never delete recycled names from a new context.
    if (deleteTextures && !textures_.empty()) glDeleteTextures(GLsizei(textures_.size()), textures_.data());
    textures_.clear(); meshes_.clear(); sequences_.clear();muzzles_.clear();label_.clear();lastFlashSerial_=0;boneCount_ = 0;
}
bool GunMesh::TexturesValid() const {
    // A focus change alone does not invalidate textures in a surviving OpenGL context.
    return !textures_.empty() && std::all_of(textures_.begin(), textures_.end(),
        [](GLuint texture) { return glIsTexture(texture)==GL_TRUE; });
}
void GunMesh::Draw(const char* animation,float seconds,float flash,float flashScale,unsigned serial,bool geometry,const char* idle) {
    if (sequences_.empty()) return;
    const Sequence* sequence = &sequences_.front();
    for (const auto& item : sequences_) if (item.name == animation) { sequence = &item; break; }
    float frame = WeaponAnimation::Frame(sequence->frames,sequence->fps,seconds,false);
    // One-shot actions settle into idle; idle itself loops smoothly.
    if (sequence->name.find("idle") == 0 || sequence->name=="spin" || sequence->name=="altfirecycle") frame = WeaponAnimation::Frame(sequence->frames,sequence->fps,seconds,true);
    else if (!WeaponAnimation::HoldLastFrame(sequence->name) && frame >= sequence->frames - 1) {
        for (const auto& item : sequences_) if (item.name.find("idle") == 0) { sequence = &item; break; }
        // The caller supplies loaded/empty idle intent; an empty RPG must not grow a rocket back visually.
        if(idle)for(const auto& item:sequences_)if(item.name==idle){sequence=&item;break;}
        frame = std::fmod(seconds * sequence->fps, float(sequence->frames));
    }
    unsigned f0 = std::min(unsigned(frame), sequence->frames - 1), f1 = std::min(f0 + 1, sequence->frames - 1);
    float weight = frame - float(f0);
    std::vector<float> bones(boneCount_ * 12);
    for (size_t i = 0; i < bones.size(); ++i) {
        float a = sequence->bones[f0 * bones.size() + i], b = sequence->bones[f1 * bones.size() + i];
        bones[i] = a + (b - a) * weight;
    }
    if(geometry)Geometry(bones,true);
    if(flash>0 && !muzzles_.empty()) {
        const auto& muzzle=muzzles_[(serial ? serial-1 : 0)%muzzles_.size()];const float* m=&bones[muzzle.bone*12];
        float x=m[0]*muzzle.x+m[1]*muzzle.y+m[2]*muzzle.z+m[3];
        float y=m[4]*muzzle.x+m[5]*muzzle.y+m[6]*muzzle.z+m[7];
        float z=m[8]*muzzle.x+m[9]*muzzle.y+m[10]*muzzle.z+m[11];
        MuzzleFlashView::Draw(y,z,-x,flash,flashScale,serial);
        if(serial!=lastFlashSerial_) {lastFlashSerial_=serial;wc3::Log("muzzle flash model=%s serial=%u point=%.2f,%.2f,%.2f",label_.c_str(),serial,y,z,-x);}
    }
}
void GunMesh::Geometry(const std::vector<float>& bones,bool viewmodel) {
    glEnable(GL_TEXTURE_2D);glColor4f(1,1,1,1);
    for (const auto& mesh : meshes_) {
        glBindTexture(GL_TEXTURE_2D, textures_[mesh.texture]); glBegin(GL_TRIANGLES);
        for (const auto& v : mesh.vertices) {
            const float* m = &bones[v.bone * 12];
            float x = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
            float y = m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7];
            float z = m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11];
            // CS viewmodels store the left-hand pose; mirror it for the default right-hand view.
            glTexCoord2f(v.u,v.v);
            if(viewmodel)glVertex3f(y,z,-x);else glVertex3f(x,y,z);
        }
        glEnd();
    }
}
void GunMesh::DrawWorld() {
    if(sequences_.empty() || meshes_.empty())return;
    const Sequence* pose=&sequences_.front();for(const auto& s:sequences_)if(s.name=="idle"){pose=&s;break;}
    std::vector<float> bones(pose->bones.begin(),pose->bones.begin()+boneCount_*12);
    float minimum[3]={1e30f,1e30f,1e30f},maximum[3]={-1e30f,-1e30f,-1e30f};
    for(const auto& mesh:meshes_)for(const auto& v:mesh.vertices) {
        const float* m=&bones[v.bone*12];float p[]={m[0]*v.x+m[1]*v.y+m[2]*v.z+m[3],m[4]*v.x+m[5]*v.y+m[6]*v.z+m[7],m[8]*v.x+m[9]*v.y+m[10]*v.z+m[11]};
        for(int i=0;i<3;++i){minimum[i]=std::min(minimum[i],p[i]);maximum[i]=std::max(maximum[i],p[i]);}
    }
    // Retail props have an offset origin; centering keeps their textured body attached to the swept projectile hull.
    glPushMatrix();glTranslatef(-(minimum[0]+maximum[0])*.5f,-(minimum[1]+maximum[1])*.5f,-(minimum[2]+maximum[2])*.5f);
    Geometry(bones,false);glPopMatrix();
}

void GunMesh::DrawPreview(float x,float y,float width,float height) {
    if (sequences_.empty() || meshes_.empty()) return;
    // Freeze the original unsilenced idle pose, then fit its actual geometry rather than fixed weapon offsets.
    const Sequence* pose=&sequences_.front();
    for (const auto& sequence : sequences_) if (sequence.name=="idle_unsil" || sequence.name=="idle1_unsil") { pose=&sequence;break; }
    struct PreviewVertex { float x,y,z,u,v; };
    std::vector<std::vector<PreviewVertex>> geometry;
    float minimum[3]={1e9f,1e9f,1e9f},maximum[3]={-1e9f,-1e9f,-1e9f};
    for (const auto& mesh : meshes_) {
        std::vector<PreviewVertex> vertices;
        for (const auto& vertex : mesh.vertices) {
            const float* m=&pose->bones[vertex.bone*12];
            float p[3]={m[0]*vertex.x+m[1]*vertex.y+m[2]*vertex.z+m[3],
                m[4]*vertex.x+m[5]*vertex.y+m[6]*vertex.z+m[7],
                m[8]*vertex.x+m[9]*vertex.y+m[10]*vertex.z+m[11]};
            // A slight side angle shows thickness while keeping the familiar horizontal CS preview.
            p[0]+=.22f*p[1];
            for (int axis=0;axis<3;++axis) { minimum[axis]=std::min(minimum[axis],p[axis]);maximum[axis]=std::max(maximum[axis],p[axis]); }
            vertices.push_back({p[0],p[1],p[2],vertex.u,vertex.v});
        }
        geometry.push_back(std::move(vertices));
    }
    float scale=std::min(width/std::max(1.0f,maximum[0]-minimum[0]),height/std::max(1.0f,maximum[2]-minimum[2]))*.86f;
    glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT|GL_TEXTURE_BIT|GL_CURRENT_BIT);
    glEnable(GL_TEXTURE_2D);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);
    glDepthRange(0,1);glClearDepth(1);glClear(GL_DEPTH_BUFFER_BIT);glColor4f(1,1,1,1);
    for (size_t i=0;i<meshes_.size();++i) {
        glBindTexture(GL_TEXTURE_2D,textures_[meshes_[i].texture]);glBegin(GL_TRIANGLES);
        for (const auto& vertex : geometry[i]) {
            glTexCoord2f(vertex.u,vertex.v);
            glVertex3f(x+width*.5f+(vertex.x-(maximum[0]+minimum[0])*.5f)*scale,
                y+height*.5f-(vertex.z-(maximum[2]+minimum[2])*.5f)*scale,
                (vertex.y-(maximum[1]+minimum[1])*.5f)/std::max(1.0f,maximum[1]-minimum[1]));
        }
        glEnd();
    }
    glPopAttrib();
}
