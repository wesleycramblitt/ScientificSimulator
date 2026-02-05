#pragma once
#include <vector>
#include <cstdint>
#include "graphics/vertex.hpp"
#include "glad/gl.h"

enum Topology { TRIANGLES, LINES, POINTS };
struct Mesh {
    //TODO need to handle submeshing, multiple indices and somehow map other material attributes to submeshes
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Topology topology = TRIANGLES;
};


struct MeshGPU {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;                 // 0 if non-indexed
    GLenum topology = GL_TRIANGLES;
    GLsizei index_count = 0;
    GLsizei vertex_count = 0;

    uint32_t layout = {};     // key for attribute setup

    void destroy() {
        if (ebo) glDeleteBuffers(1, &ebo), ebo = 0;
        if (vbo) glDeleteBuffers(1, &vbo), vbo = 0;
        if (vao) glDeleteVertexArrays(1, &vao), vao = 0;
    }
};
