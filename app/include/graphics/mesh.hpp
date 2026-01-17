#pragma once
#include <vector>
#include <cstdint>
#include "graphics/vertex.hpp"

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
