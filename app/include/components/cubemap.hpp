#pragma once
#include "graphics/texture.hpp"
#include <vector>
#include <string>

struct CubeMap {
    std::string name;
    bool cross_layout = false;   // single cross-shaped image instead of 6 separate files
    std::vector<Texture> faces;
    std::uint32_t texture_handle = -1;
};
