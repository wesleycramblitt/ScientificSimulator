#include "systems/volume_render_system.hpp"
#include "components/transform.hpp"
#include "components/camera.hpp"
#include "components/renderable.hpp"
#include "components/volume_renderable.hpp"
#include "components/fluid_domain.hpp"
#include "components/volume_field.hpp"
#include "components/disabled.hpp"
#include "components/particle_cloud.hpp"
#include "graphics/vertex.hpp"
#include "math/vec3.hpp"
#include "math/mat4.hpp"
#include "math/quat.hpp"

#include <glad/gl.h>
#include <cstdio>
#include <algorithm>

namespace exd {
namespace systems {

VolumeRenderSystem::VolumeRenderSystem(graphics::GraphicsContext& graphicsContext)
    : graphicsContext_(graphicsContext)
{}

// ---------------------------------------------------------------------------
// Compute world-space AABB of the simulation domain
// ---------------------------------------------------------------------------
void VolumeRenderSystem::computeWorldBounds(const components::Transform& xform,
                                            int nx, int ny, int nz,
                                            math::Vec3& out_min, math::Vec3& out_max) {
    const float hx = (float)nx * 0.5f;
    const float hy = (float)ny * 0.5f;
    const float hz = (float)nz * 0.5f;

    // 8 corners of the domain in grid-space
    const math::Vec3 corners[8] = {
        math::Vec3{-hx, -hy, -hz}, math::Vec3{ hx, -hy, -hz},
        math::Vec3{-hx,  hy, -hz}, math::Vec3{ hx,  hy, -hz},
        math::Vec3{-hx, -hy,  hz}, math::Vec3{ hx, -hy,  hz},
        math::Vec3{-hx,  hy,  hz}, math::Vec3{ hx,  hy,  hz},
    };

    // Transform to world-space and find min/max
    out_min = math::Vec3{ 1e30f, 1e30f, 1e30f};
    out_max = math::Vec3{-1e30f,-1e30f,-1e30f};
    for (const auto& c : corners) {
        // Apply TRS manually
        math::Vec3 rotated = xform.rotation * math::Vec3{c.x * xform.scale.x,
                                              c.y * xform.scale.y,
                                              c.z * xform.scale.z};
        math::Vec3 world = math::Vec3{rotated.x + xform.position.x,
                          rotated.y + xform.position.y,
                          rotated.z + xform.position.z};
        out_min.x = std::min(out_min.x, world.x);
        out_min.y = std::min(out_min.y, world.y);
        out_min.z = std::min(out_min.z, world.z);
        out_max.x = std::max(out_max.x, world.x);
        out_max.y = std::max(out_max.y, world.y);
        out_max.z = std::max(out_max.z, world.z);
    }
}

// ---------------------------------------------------------------------------
// Create a solid cube mesh for the proxy geometry (12 triangles, 36 vertices)
// The cube matches the domain's grid-space bounds so the TRS model matrix
// places it correctly in world-space.
// ---------------------------------------------------------------------------
void VolumeRenderSystem::createProxyCube(entities::Registry& registry, entities::Entity e) {
    auto& domain = registry.get<components::SimulationDomain>(e);
    const float hx = (float)domain.nx * 0.5f;
    const float hy = (float)domain.ny * 0.5f;
    const float hz = (float)domain.nz * 0.5f;

    graphics::Mesh mesh;
    mesh.topology = graphics::TRIANGLES;

    // 8 corner positions
    const math::Vec3 p[8] = {
        {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, {-hx,  hy, -hz},
        {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz},
    };

    // 6 faces, 2 triangles each, CCW winding for GL_CULL_FACE
    auto tri = [&](int a, int b, int c) {
        math::Vec3 u = math::Vec3{p[b].x-p[a].x, p[b].y-p[a].y, p[b].z-p[a].z};
        math::Vec3 v = math::Vec3{p[c].x-p[a].x, p[c].y-p[a].y, p[c].z-p[a].z};
        math::Vec3 n{u.y*v.z - u.z*v.y, u.z*v.x - u.x*v.z, u.x*v.y - u.y*v.x};
        // Normal is unused by ray-march shader but needed for valid Vertex
        mesh.vertices.push_back({p[a], n});
        mesh.vertices.push_back({p[b], n});
        mesh.vertices.push_back({p[c], n});
    };

    // +Z face (front), -Z (back), +X (right), -X (left), +Y (top), -Y (bottom)
    tri(4, 5, 6); tri(4, 6, 7);  // +Z
    tri(1, 0, 3); tri(1, 3, 2);  // -Z
    tri(5, 1, 2); tri(5, 2, 6);  // +X
    tri(0, 4, 7); tri(0, 7, 3);  // -X
    tri(7, 6, 2); tri(7, 2, 3);  // +Y
    tri(0, 1, 5); tri(0, 5, 4);  // -Y

    uint32_t handle = graphicsContext_.mesh_manager.create(mesh);
    registry.emplace<components::VolumeRenderable>(e, handle);
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------
void VolumeRenderSystem::update(entities::Registry& registry, const core::Window& window, float /*dt*/) {
    // --- Find camera ---
    const components::Transform* cam_xform = nullptr;
    const components::Camera* cam = nullptr;
    for (auto e : registry.view<components::Camera, components::Transform>()) {
        cam = &registry.get<components::Camera>(e);
        cam_xform = &registry.get<components::Transform>(e);
        break;
    }
    if (!cam || !cam_xform) return;

    int width, height; float aspect;
    window.getDimensions(width, height, aspect);

    math::Vec3 forward = (cam_xform->rotation * math::Vec3{0.0f, 0.0f, -1.0f}).norm();
    math::Vec3 up      = (cam_xform->rotation * math::Vec3{0.0f, 1.0f,  0.0f}).norm();
    math::Mat4 view = math::Mat4::lookAt(cam_xform->position,
                             cam_xform->position + forward, up);
    math::Mat4 proj = math::Mat4::perspective(cam->fov_y_radians, aspect,
                                   cam->near_plane, cam->far_plane);

        // --- Volume entities ---
        for (auto e : registry.view<components::Transform, components::VolumeField, components::SimulationDomain>()) {
            if (registry.has<components::Disabled>(e)) continue;

            auto& transform = registry.get<components::Transform>(e);
            auto& vol       = registry.get<components::VolumeField>(e);
            auto& domain    = registry.get<components::SimulationDomain>(e);

        if (!vol.interop_ready || vol.texture_handle == 0)
            continue;

            // Lazy-create proxy cube mesh
            if (!registry.has<components::VolumeRenderable>(e))
                createProxyCube(registry, e);

            auto& vr = registry.get<components::VolumeRenderable>(e);
            if (vr.mesh == 0) continue;

        // Compute world-space bounds
        math::Vec3 box_min, box_max;
        computeWorldBounds(transform, domain.nx, domain.ny, domain.nz,
                           box_min, box_max);

        // --- Shader ---
        static bool shader_loaded = false;
        GLuint prog = graphicsContext_.shader_manager.getOrLoad(
            "volume_ray",
            "shaders/volume/ray_march.vert",
            "shaders/volume/ray_march.frag");
        if (!shader_loaded) {
            printf("[Volume] Shader program: %u\n", prog);
            shader_loaded = true;
        }
        glUseProgram(prog);

        math::Mat4 model = math::Mat4::modelTRS(transform.position, transform.rotation,
                                    transform.scale);

        glUniformMatrix4fv(glGetUniformLocation(prog, "u_model"), 1, GL_FALSE, model.m);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_view"),  1, GL_FALSE, view.m);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"),  1, GL_FALSE, proj.m);
        glUniform3f(glGetUniformLocation(prog, "u_cam_pos"),
                    cam_xform->position.x,
                    cam_xform->position.y,
                    cam_xform->position.z);
        glUniform3f(glGetUniformLocation(prog, "u_box_min"),
                    box_min.x, box_min.y, box_min.z);
        glUniform3f(glGetUniformLocation(prog, "u_box_max"),
                    box_max.x, box_max.y, box_max.z);
        glUniform3i(glGetUniformLocation(prog, "u_grid_dims"),
                    domain.nx, domain.ny, domain.nz);

        // Bind 3D texture via the texture manager
        graphicsContext_.texture_manager.bind(vol.texture_handle);
        glUniform1i(glGetUniformLocation(prog, "u_volume"), 0);

        // Draw proxy cube (back faces for camera-inside-volume case)
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);  // render back faces so camera-inside works

        const graphics::MeshGPU* mesh = graphicsContext_.mesh_manager.bind(vr.mesh);
        glDrawArrays(mesh->topology, 0, (GLsizei)mesh->vertex_count);

        glCullFace(GL_BACK);
        glBindTexture(GL_TEXTURE_3D, 0);
    }

    // ── Particle pass ──
    for (auto e : registry.view<components::Transform, components::ParticleCloud, components::SimulationDomain>()) {
        if (registry.has<components::Disabled>(e)) continue;
        auto& pc = registry.get<components::ParticleCloud>(e);
        if (!pc.initialized || pc.particle_count == 0) continue;
        auto& transform = registry.get<components::Transform>(e);

        math::Mat4 model = math::Mat4::modelTRS(transform.position, transform.rotation, transform.scale);

        GLuint prog = graphicsContext_.shader_manager.getOrLoad(
            "particle_points",
            "shaders/particle/particle.vert",
            "shaders/particle/particle.frag");
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_model"), 1, GL_FALSE, model.m);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_view"),  1, GL_FALSE, view.m);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"),  1, GL_FALSE, proj.m);

        glBindVertexArray(pc.gl_vao);
        glPointSize(2.0f);
        glDrawArrays(GL_POINTS, 0, pc.particle_count);
        glBindVertexArray(0);
    }
}

} // namespace systems
} // namespace exd
