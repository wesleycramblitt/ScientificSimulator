#include "systems/particle_system.hpp"
#include "components/transform.hpp"
#include "components/camera.hpp"
#include "components/fluid_domain.hpp"
#include "components/simulation_status.hpp"
#include "components/disabled.hpp"
#include "math/mat4.hpp"
#include "math/quat.hpp"

#define Mesh F3D_Mesh
#include "lbm.hpp"
#undef Mesh

#include <glad/gl.h>
#include <cstdio>
#include <vector>

ParticleSystem::ParticleSystem() = default;

void ParticleSystem::initGL(int particle_count) {
    glGenVertexArrays(1, &gl_vao_);
    glGenBuffers(1, &gl_vbo_);
    glBindVertexArray(gl_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo_);
    glBufferData(GL_ARRAY_BUFFER, (size_t)particle_count * 3 * sizeof(float),
                 nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    gl_particle_count_ = particle_count;
    gl_initialized_ = true;
    printf("[ParticleSys] Initialized %d particles\n", particle_count);
}

void ParticleSystem::uploadParticles(LBM* lbm, int& out_count) {
    if (!lbm || !lbm->particles) { out_count = 0; return; }
    lbm->particles->read_from_device();
    ulong Np = lbm->particles->length();
    if (Np == 0) { out_count = 0; return; }
    if (!gl_initialized_ || (int)Np != gl_particle_count_)
        initGL((int)Np);

    std::vector<float> interleaved(Np * 3);
    for (ulong i = 0; i < Np; i++) {
        interleaved[i*3+0] = lbm->particles->x[i];
        interleaved[i*3+1] = lbm->particles->y[i];
        interleaved[i*3+2] = lbm->particles->z[i];
    }
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, Np * 3 * sizeof(float), interleaved.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    out_count = (int)Np;
}

void ParticleSystem::update(Registry& registry, const Window& window, float /*dt*/, LBM* lbm) {
    if (!lbm) return;

    // Find camera
    const Transform* cam_xform = nullptr;
    const Camera* cam = nullptr;
    for (auto e : registry.view<Camera, Transform>()) {
        cam = &registry.get<Camera>(e);
        cam_xform = &registry.get<Transform>(e);
        break;
    }
    if (!cam || !cam_xform) return;

    int w, h; float aspect;
    window.getDimensions(w, h, aspect);

    Vec3 forward = (cam_xform->rotation * Vec3{0.0f, 0.0f, -1.0f}).norm();
    Vec3 up      = (cam_xform->rotation * Vec3{0.0f, 1.0f,  0.0f}).norm();
    Mat4 view_mat = Mat4::lookAt(cam_xform->position, cam_xform->position + forward, up);
    Mat4 proj_mat = Mat4::perspective(cam->fov_y_radians, aspect, cam->near_plane, cam->far_plane);

    // Find domain entity
    for (auto e : registry.view<Transform, SimulationDomain, SimulationInfo>()) {
        if (registry.has<Disabled>(e)) continue;
        if (registry.get<SimulationInfo>(e).status != SimulationStatus::Running) continue;

        int count = 0;
        uploadParticles(lbm, count);
        if (count == 0) continue;

        auto& xform = registry.get<Transform>(e);
        Mat4 model = Mat4::modelTRS(xform.position, xform.rotation, xform.scale);

        GLuint prog = shader_manager_.getOrLoad(
            "particle_points",
            "shaders/particle/particle.vert",
            "shaders/particle/particle.frag");
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_model"), 1, GL_FALSE, model.m);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_view"),  1, GL_FALSE, view_mat.m);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"),  1, GL_FALSE, proj_mat.m);

        glBindVertexArray(gl_vao_);
        glDrawArrays(GL_POINTS, 0, count);
        glBindVertexArray(0);
    }
}
