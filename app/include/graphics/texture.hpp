#pragma once
#include <string>
#include <cstdint>

struct Texture {
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;

    uint32_t mipLevels = 1;
    std::string name;
};
