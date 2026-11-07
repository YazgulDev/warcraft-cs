#include "ModelBounds.hpp"
#include "../platform/WarcraftApi.hpp"
#include "../platform/StormFile.hpp"
#include "TreeTrunkMesh.hpp"
#include "StaticModelMesh.hpp"
#include <cstring>
#include <vector>

namespace {
bool Valid(const Bounds3& b) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(b.minimum[i]) || !std::isfinite(b.maximum[i]) ||
            b.maximum[i] <= b.minimum[i] || b.maximum[i] - b.minimum[i] > 6000) return false;
    return true;
}
bool ModelBytes(const std::string& path, std::vector<unsigned char>& bytes) {
    return StormFile::Read(path.c_str(), bytes, 32 * 1024 * 1024);
}
bool Extents(const std::vector<unsigned char>& bytes, Bounds3& result) {
    if (bytes.size() < 4 || memcmp(bytes.data(), "MDLX", 4)) return false;
    bool found = false;
    // Stand extents exclude death/spell particles that can greatly inflate a model's global box.
    for (size_t at = 4; at + 8 <= bytes.size();) {
        unsigned length = 0; memcpy(&length, bytes.data() + at + 4, 4);
        size_t body = at + 8;
        if (length > bytes.size() - body) break;
        if (!memcmp(bytes.data() + at, "MODL", 4) && length >= 372) {
            Bounds3 b; memcpy(&b, bytes.data() + body + 344, sizeof(b));
            if (Valid(b)) { result = b; found = true; }
        }
        if (!memcmp(bytes.data() + at, "SEQS", 4)) {
            for (size_t seq = body; seq + 132 <= body + length; seq += 132) {
                if (_strnicmp(reinterpret_cast<const char*>(bytes.data() + seq), "Stand", 5)) continue;
                Bounds3 b; memcpy(&b, bytes.data() + seq + 108, sizeof(b));
                if (Valid(b)) { result = b; return true; }
            }
        }
        at = body + length;
    }
    return found;
}
}
bool ModelBounds::Load(std::string path, Bounds3& result, TreeTrunkMesh* tree) {
    // Warcraft object files may name MDL sources; installed archives contain the compiled MDX.
    path = path.substr(0, path.find(','));
    size_t extension = path.find_last_of('.');
    if (extension != std::string::npos) path.resize(extension);
    path += ".mdx";
    std::vector<unsigned char> bytes;
    if (!ModelBytes(path, bytes) || !Extents(bytes, result)) return false;
    // Decode trunk surfaces from the same active-map model snapshot used for broad bounds.
    if (tree) tree->Load(path,bytes,result);
    return true;
}
bool ModelBounds::LoadCollision(std::string path,CylinderMesh& mesh) {
    // Reuse the mounted-map lookup; collision must follow custom imported geometry too.
    path=path.substr(0,path.find(','));
    size_t extension=path.find_last_of('.');
    if(extension!=std::string::npos)path.resize(extension);
    std::vector<unsigned char> bytes;
    return ModelBytes(path+".mdx",bytes) && StaticModelMesh::Load(bytes,mesh);
}
bool ModelBounds::ReadModel(std::string path,std::vector<unsigned char>& bytes) {
    path=path.substr(0,path.find(','));
    size_t extension=path.find_last_of('.');if(extension!=std::string::npos)path.resize(extension);
    return !path.empty() && ModelBytes(path+".mdx",bytes);
}
