#pragma once
#include "components/volume_field.hpp"
#include <cstdio>
#include <vector>
#include <cmath>
#include <glad/gl.h>

// Helper for 3D volume texture operations.
// Keeps GL code out of solver systems — just pass raw float data.
struct VolumeTexture {
    // Create a 3D R32F texture. Call once.
    static void create(VolumeField& vf, int nx, int ny, int nz) {
        glGenTextures(1, &vf.gl_tex);
        glBindTexture(GL_TEXTURE_3D, vf.gl_tex);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        float border[4] = {0,0,0,0};
        glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, border);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, nx, ny, nz, 0, GL_RED, GL_FLOAT, nullptr);
        glBindTexture(GL_TEXTURE_3D, 0);
        vf.interop_ready = true;
        printf("[VolumeTex] Created %dx%dx%d (gl_tex=%u)\n", nx, ny, nz, vf.gl_tex);
    }

    // Upload scalar data.  data[i] = value at grid cell i.
    static void upload(VolumeField& vf, int nx, int ny, int nz,
                       const float* data) {
        glBindTexture(GL_TEXTURE_3D, vf.gl_tex);
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz,
                        GL_RED, GL_FLOAT, data);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
};
