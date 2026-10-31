#pragma once
#include "../config/ConfigChange.hpp"
#include <vector>

// Menu writes are a sibling-file transaction; resource discovery never imports or downloads new assets.
namespace SettingsStore {
bool Save(const std::string& path,const std::vector<ConfigChange>& changes);
std::vector<std::string> Skies(const std::string& root);
}
