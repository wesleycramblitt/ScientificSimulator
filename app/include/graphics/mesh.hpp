#pragma once
#include <vector>
#include <cstdint>
#include "graphics/vertex.hpp"

enum Topology { TRIANGLES, LINES, POINTS };

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Topology topology = TRIANGLES;
};


