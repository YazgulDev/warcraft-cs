#pragma once
#include <windows.h>
#include "GraphicsApi.hpp"
#include <string>
#include <unordered_map>

// Spell and inventory art share map-scoped texture ownership and missing-art caching.
class HudIconCache {
public:
    void Reset(bool deleteObjects);
    bool Valid() const;
    GLuint Texture(const std::string& path);
private:
    std::unordered_map<std::string,GLuint> icons_;
};
