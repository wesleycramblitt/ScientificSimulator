#pragma once
#include "common/macros.hpp"
#include <vector>
#include <cstdint>
#include "graphics/vertex.hpp"
#include "glad/gl.h"
#include <cstddef>
#include <iostream>

enum Topology { TRIANGLES, LINES, POINTS };
struct Mesh {
    //TODO need to handle submeshing, multiple indices and somehow map other material attributes to submeshes
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Topology topology = TRIANGLES;
};


struct MeshGPU {
    MeshGPU(const Mesh& _mesh) { generateArraysAndBuffers(); upload(_mesh); }
    
    ~MeshGPU() { 
        std::cout << "MeshGPU destroy" << std::endl;
        destroy(); }
    MeshGPU(const MeshGPU&) = delete;
    MeshGPU& operator=(const MeshGPU&) = delete;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;                 // 0 if non-indexed
    GLenum topology = GL_TRIANGLES;
    GLsizei index_count = 0;
    GLsizei vertex_count = 0;

    uint32_t layout = {};     // key for attribute setup

    void  upload(Mesh mesh) {
        switch (mesh.topology) {
            case LINES:     topology = GL_LINES;     break;
            case POINTS:    topology = GL_POINTS;    break;
            default:        topology = GL_TRIANGLES; break;
        }

        index_count = mesh.indices.size();
        vertex_count = mesh.vertices.size();
        GL_CALL(glBindVertexArray(vao));
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        GL_CALL(glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size()*sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW));

        if (index_count > 0) {
            GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
            GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size()*sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW));
        } 

        // TODO dynamic layout for other shaders
        // layout(locaiton=0) vec3 a_pos;
        GL_CALL(glVertexAttribPointer(
                0,                 // attribute location
                3,                 // components (x,y,z)
                GL_FLOAT,          // type
                GL_FALSE,          // normalized
                6 * sizeof(float), // stride (bytes per vertex)
                (void*)0           // offset
        ));
        GL_CALL(glEnableVertexAttribArray(0));

        // layout(location=1) vec3 a_norm;
        GL_CALL(glVertexAttribPointer(
                1,                 // attribute location
                3,                 // components (x,y,z)
                GL_FLOAT,          // type
                GL_FALSE,          // normalized
                6 * sizeof(float), // stride (bytes per vertex)
                (void*)offsetof(Vertex, normal)           // offset
        ));
        GL_CALL(glEnableVertexAttribArray(1));

        // layout(location=3) vec3 a_uv;
        // glVertexAttribPointer(
        //         2,                 // attribute location
        //         3,                 // components (x,y,z)
        //         GL_FLOAT,          // type
        //         GL_FALSE,          // normalized
        //         9 * sizeof(float), // stride (bytes per vertex)
        //         (void*)offsetof(Vertex, uv)           // offset
        // );
        // glEnableVertexAttribArray(2);
        
        GL_CALL(glBindVertexArray(0));

        // Note: do NOT unbind GL_ELEMENT_ARRAY_BUFFER while VAO is bound unless you intend to clear it.
        // (Unbinding EBO with VAO bound will make VAO remember "no EBO".)
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));    

        // std::cout << "After mesh upload to gpu" << std::endl;
    }

    void bind() {
        GL_CALL(glBindVertexArray(vao));
    }

    void generateArraysAndBuffers() {
        GL_CALL(glGenVertexArrays(1, &vao));
        GL_CALL(glGenBuffers(1, &vbo));
        GL_CALL(glGenBuffers(1, &ebo));
        std::cout << "vao: " << vao << " vbo: " << vbo << " ebo: " << ebo << std::endl;
    }

    void destroy() {
        if (ebo) glDeleteBuffers(1, &ebo), ebo = 0;
        if (vbo) glDeleteBuffers(1, &vbo), vbo = 0;
        if (vao) glDeleteVertexArrays(1, &vao), vao = 0;
    }
};
