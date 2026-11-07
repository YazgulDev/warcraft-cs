#include "MapRockObstacles.hpp"
#include "StormFile.hpp"
#include "WarcraftApi.hpp"
#include "../geometry/ModelBounds.hpp"
#include <charconv>
#include <cctype>
#include <cstring>
#include <map>
#include <string_view>

namespace {
struct RockType {
    std::string model;
    std::string_view pathing;
    unsigned variations = 0;
    bool walkable = false;
};

unsigned Number(std::string_view value) {
    unsigned result = 0;
    std::from_chars(value.data(), value.data() + value.size(), result);
    return result;
}

std::map<std::string, RockType> RockTypes(const std::vector<unsigned char>& table) {
    const std::string_view slk(reinterpret_cast<const char*>(table.data()), table.size());
    std::map<std::string, RockType> types;
    RockType* row = nullptr;
    // The stock doodad table supplies model variants and tells us which rocks occupy RTS pathing.
    for (size_t start = 0; start < slk.size();) {
        size_t end = slk.find('\n', start);
        if (end == slk.npos) end = slk.size();
        auto line = slk.substr(start, end - start);
        start = end + 1;
        const size_t valueAt = line.find(";K");
        if (line.substr(0, 3) != "C;X" || valueAt == line.npos) continue;
        unsigned column = Number(line.substr(3, valueAt - 3));
        auto value = line.substr(valueAt + 2);
        if (!value.empty() && value.back() == '\r') value.remove_suffix(1);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        if (column == 1) row = value.size() == 4 ? &types[std::string(value)] : nullptr;
        if (!row) continue;
        if (column == 5) row->model = value;
        if (column == 20) row->walkable = value == "1";
        if (column == 21) row->variations = Number(value);
        if (column == 29) row->pathing = value;
    }
    return types;
}

template<typename T>
T Read(const std::vector<unsigned char>& data, size_t at) {
    T value;
    std::memcpy(&value, data.data() + at, sizeof(value));
    return value;
}

bool SkipItemSets(const std::vector<unsigned char>& data, size_t& at) {
    if (data.size() - at < 8) return false;
    unsigned sets = Read<unsigned>(data, at + 4);
    at += 8;
    for (unsigned set = 0; set < sets; ++set) {
        if (data.size() - at < 4) return false;
        unsigned items = Read<unsigned>(data, at);
        at += 4;
        if (items > (data.size() - at) / 8) return false;
        at += size_t(items) * 8;
    }
    return true;
}

bool IsRock(const RockType& type) {
    if (type.walkable || type.pathing.empty() || type.pathing == "_" || type.pathing == "-") return false;
    std::string path = type.model;
    for (char& c : path) c = c == '/' ? '\\' : char(std::tolower(static_cast<unsigned char>(c)));
    return path.find("\\rocks\\") != path.npos;
}

bool ValidPlacement(const float (&pose)[7]) {
    for (float value : pose) if (!std::isfinite(value) || std::abs(value) > 100000) return false;
    for (int axis = 4; axis < 7; ++axis) if (pose[axis] <= 0 || pose[axis] > 100) return false;
    return true;
}

Bounds3 MeshBounds(const CylinderMesh& mesh) {
    Bounds3 bounds;
    std::copy_n(mesh.triangles.front().vertices[0], 3, bounds.minimum);
    std::copy_n(bounds.minimum, 3, bounds.maximum);
    for (const auto& triangle : mesh.triangles) for (const auto& point : triangle.vertices)
        for (int axis = 0; axis < 3; ++axis) {
            bounds.minimum[axis] = std::min(bounds.minimum[axis], point[axis]);
            bounds.maximum[axis] = std::max(bounds.maximum[axis], point[axis]);
        }
    return bounds;
}
}

void MapRockObstacles::Load() {
    loaded_ = true;
    std::vector<unsigned char> placements, table;
    if (!StormFile::Read("war3map.doo", placements, 8 * 1024 * 1024) || placements.size() < 16 ||
        std::memcmp(placements.data(), "W3do", 4)) return;
    const unsigned version = Read<unsigned>(placements, 4);
    const unsigned subversion = Read<unsigned>(placements, 8);
    const unsigned count = Read<unsigned>(placements, 12);
    if (!count || count > 65536 || !((version == 7 && subversion == 9) ||
        (version == 8 && subversion == 11))) return;
    if (!StormFile::Read("Doodads\\Doodads.slk", table, 4 * 1024 * 1024)) return;
    const auto types = RockTypes(table);
    std::map<std::string, CylinderMesh> models;
    std::vector<Rock> rocks;
    size_t triangles = 0;
    for (size_t at = 16, index = 0; index < count; ++index) {
        // RoC and TFT share the placement prefix. TFT adds item-drop sets before the editor ID.
        if (placements.size() - at < 42) return;
        const std::string code(reinterpret_cast<const char*>(placements.data() + at), 4);
        const unsigned variant = Read<unsigned>(placements, at + 4);
        float pose[7];
        std::memcpy(pose, placements.data() + at + 8, sizeof(pose));
        const unsigned flags = placements[at + 36];
        at += 38;
        if ((version == 8 && !SkipItemSets(placements, at)) || placements.size() - at < 4) return;
        at += 4;
        // Ignore hidden/non-solid doodads and keep the real yaw, per-axis scale and authored Z.
        if ((flags & 3) != 2 || !ValidPlacement(pose)) continue;
        auto type = types.find(code);
        if (type == types.end() || !IsRock(type->second) || variant >= type->second.variations) continue;
        auto path = type->second.model;
        if (type->second.variations > 1) path += std::to_string(variant);
        auto [model, inserted] = models.try_emplace(path);
        if (inserted) ModelBounds::LoadCollision(path, model->second);
        const auto& mesh = model->second;
        // Unreadable models keep the normal native pathing fallback. Bound map-owned mesh memory.
        if (mesh.triangles.empty() || mesh.triangles.size() > 250000 - triangles) continue;
        float cosine = std::cos(pose[3]), sine = std::sin(pose[3]);
        ModelTransform transform{{cosine * pose[4], -sine * pose[5], 0,
            sine * pose[4], cosine * pose[5], 0, 0, 0, pose[6]}, {pose[0], pose[1], pose[2]}};
        auto placed = std::make_shared<CylinderMesh>(mesh.Transformed(transform));
        auto bounds = MeshBounds(*placed);
        rocks.push_back({bounds, std::move(placed)});
        triangles += mesh.triangles.size();
    }
    rocks_ = std::move(rocks);
    wc3::Log("movement map rocks=%u triangles=%u", unsigned(rocks_.size()), unsigned(triangles));
}

void MapRockObstacles::Append(float x, float y, float radius, std::vector<Bounds3>& bounds,
    std::vector<std::shared_ptr<const CylinderMesh>>& geometry) {
    if (!loaded_) Load();
    for (const auto& rock : rocks_) {
        const auto& box = rock.bounds;
        if (box.maximum[0] < x - radius || box.minimum[0] > x + radius ||
            box.maximum[1] < y - radius || box.minimum[1] > y + radius) continue;
        bounds.push_back(box);
        geometry.push_back(rock.mesh);
    }
}
