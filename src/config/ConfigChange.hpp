#pragma once
#include <string>

// The menu submits only changed INI keys; unrelated settings and comments remain owned by the file.
struct ConfigChange {std::string section,key,value;};
