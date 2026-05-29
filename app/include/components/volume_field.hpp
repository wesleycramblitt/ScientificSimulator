#pragma once
#include <cstdint>

// Attached to domain entity to enable volume rendering of a solver field.
// Shared 3D texture between OpenGL and OpenCL via clCreateFromGLTexture.
struct VolumeField {
    int field_id   = 1;      // FLUIDX3D_FIELD_U=1 (velocity)
    int component  = 0;      // 0=x, 1=y, 2=z (scalar component to visualize)

    // Shared GL↔CL 3D texture
    uint32_t gl_tex       = 0;   // OpenGL 3D texture (GL_TEXTURE_3D, R32F)
    intptr_t cl_gl_img    = 0;   // cl_mem wrapping gl_tex (via clCreateFromGLTexture)
    intptr_t cl_queue     = 0;   // cached command queue (from solver)
    intptr_t cl_context   = 0;   // cached OpenCL context

    uint64_t buf_size_bytes = 0;
    bool     interop_ready  = false;
};

