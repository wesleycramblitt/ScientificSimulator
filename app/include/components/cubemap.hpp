#pragma once
#include "graphics/texture.hpp"
#include <vector>
#include <string>

struct CubeMap {
    std::string name;
    std::vector<Texture> faces;
    std::uint32_t texture_handle = -1;
};
