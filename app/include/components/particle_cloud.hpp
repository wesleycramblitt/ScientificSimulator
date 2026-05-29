#pragma once
#include <cstdint>

// Particle tracer cloud attached to domain entity
struct ParticleCloud {
    uint32_t gl_vbo = 0;       // OpenGL VBO with particle positions (x,y,z)
    uint32_t gl_vao = 0;       // VAO for rendering
    int particle_count = 0;
    bool initialized = false;
};
