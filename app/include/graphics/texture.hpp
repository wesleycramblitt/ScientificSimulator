#pragma once
#include <string>
#include <cstdint>
#include <glad/gl.h>
#include "common/macros.hpp"


struct Texture {
    std::string name;
    int width;
    int height;
    int depth = 1;
    int channels; 
    int mipLevels = 1;
};

