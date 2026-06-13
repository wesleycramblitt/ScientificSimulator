#include "systems/render_system.hpp"
#include "common/macros.hpp"
#include "components/transform.hpp"
#include "components/camera.hpp"
#include "components/renderable.hpp"
#include "components/cubemap.hpp"
#include "components/disabled.hpp"
#include "components/render_technique_mirror.hpp"
#include "components/render_technique_cubemap.hpp"
#include "components/render_technique_lambertian.hpp"
#include "components/fluid_domain.hpp"
#include "components/particle_cloud.hpp"
#include "components/volume_field.hpp"
#include "components/volume_renderable.hpp"
#include "graphics/render_techniques/renderable.hpp"
#include "graphics/render_techniques/particle_draw_data.hpp"
#include "graphics/render_techniques/volume_draw_data.hpp"
#include "core/window.hpp"
#include "math/mat4.hpp"
#include "math/quat.hpp"
#include <algorithm>

namespace exd {
namespace systems {

// ── construction ──────────────────────────────────────────────────────────

RenderSystem::RenderSystem(graphics::GraphicsContext& graphicsContext)
    : ctx_(graphicsContext),
      cubemapTechnique_(graphicsContext),
      lambertianTechnique_(graphicsContext),
      reflectiveTechnique_(graphicsContext),
      particleTechnique_(graphicsContext),
      volumeTechnique_(graphicsContext)
{}

RenderSystem::~RenderSystem() = default;

static math::Vec3 computeWorldBounds(const components::Transform& xform,
                                      int nx, int ny, int nz,
                                      math::Vec3& box_min, math::Vec3& box_max) {
    const float hx = (float)nx * 0.5f, hy = (float)ny * 0.5f, hz = (float)nz * 0.5f;
    const math::Vec3 corners[8] = {
        {-hx,-hy,-hz},{ hx,-hy,-hz},{-hx, hy,-hz},{ hx, hy,-hz},
        {-hx,-hy, hz},{ hx,-hy, hz},{-hx, hy, hz},{ hx, hy, hz},
    };
    box_min = { 1e30f, 1e30f, 1e30f};
    box_max = {-1e30f,-1e30f,-1e30f};
    for (auto& c : corners) {
        math::Vec3 r = xform.rotation * math::Vec3{c.x*xform.scale.x, c.y*xform.scale.y, c.z*xform.scale.z};
        math::Vec3 w{r.x+xform.position.x, r.y+xform.position.y, r.z+xform.position.z};
        box_min = {std::min(box_min.x,w.x), std::min(box_min.y,w.y), std::min(box_min.z,w.z)};
        box_max = {std::max(box_max.x,w.x), std::max(box_max.y,w.y), std::max(box_max.z,w.z)};
    }
    return box_max - box_min;
}

void RenderSystem::ensureVolumeProxy(entities::Registry& registry,
                                      entities::Entity e, int nx, int ny, int nz) {
    if (registry.has<components::VolumeRenderable>(e)) return;

    const float hx = (float)nx * 0.5f, hy = (float)ny * 0.5f, hz = (float)nz * 0.5f;
    graphics::Mesh mesh;
    mesh.topology = graphics::TRIANGLES;

    const math::Vec3 p[8] = {
        {-hx,-hy,-hz},{ hx,-hy,-hz},{ hx, hy,-hz},{-hx, hy,-hz},
        {-hx,-hy, hz},{ hx,-hy, hz},{ hx, hy, hz},{-hx, hy, hz},
    };
    auto tri = [&](int a, int b, int c) {
        math::Vec3 u{p[b].x-p[a].x, p[b].y-p[a].y, p[b].z-p[a].z};
        math::Vec3 v{p[c].x-p[a].x, p[c].y-p[a].y, p[c].z-p[a].z};
        math::Vec3 n{u.y*v.z - u.z*v.y, u.z*v.x - u.x*v.z, u.x*v.y - u.y*v.x};
        mesh.vertices.push_back({p[a], n});
        mesh.vertices.push_back({p[b], n});
        mesh.vertices.push_back({p[c], n});
    };
    tri(4,5,6); tri(4,6,7); tri(1,0,3); tri(1,3,2);
    tri(5,1,2); tri(5,2,6); tri(0,4,7); tri(0,7,3);
    tri(7,6,2); tri(7,2,3); tri(0,1,5); tri(0,5,4);

    uint32_t handle = ctx_.mesh_manager.create(mesh);
    registry.emplace<components::VolumeRenderable>(e, handle);
}

void RenderSystem::renderCubemapPass(entities::Registry& registry,
                                      const math::Mat4& view,
                                      const math::Mat4& proj) {
    auto v = registry.view<components::CubeMap, components::Renderable,
                            components::Render_Technique_CubeMap>();
    if (v.begin() == v.end()) return;

    cubemapTechnique_.bind();
    for (auto e : v) {
        if (registry.has<components::Disabled>(e)) continue;
        auto& cubemap = registry.get<components::CubeMap>(e);
        auto& r       = registry.get<components::Renderable>(e);
        graphics::render_techniques::Renderable data{
            r.mesh, cubemap.texture_handle,
            {{"u_view", view}, {"u_proj", proj}}
        };
        cubemapTechnique_.draw(data);
    }
    cubemapTechnique_.unbind();
}

void RenderSystem::renderOpaquePass(entities::Registry& registry,
                                     const math::Mat4& view, const math::Mat4& proj) {
    auto v = registry.view<components::Transform, components::Renderable,
                            components::Render_Technique_Lambertian>();
    if (v.begin() == v.end()) return;

    lambertianTechnique_.bind(view, proj);
    for (auto e : v) {
        if (registry.has<components::Disabled>(e)) continue;
        auto& xform = registry.get<components::Transform>(e);
        auto& r     = registry.get<components::Renderable>(e);
        if (r.mesh == 0) continue;
        math::Mat4 model = math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale);
        lambertianTechnique_.draw(r.mesh, model);
    }
    lambertianTechnique_.unbind();
}

void RenderSystem::renderReflectivePass(entities::Registry& registry,
                                         const math::Mat4& view,
                                         const math::Mat4& proj,
                                         const math::Vec3& cam_pos) {
    auto v = registry.view<components::Transform, components::Renderable,
                            components::Render_Technique_Mirror>();
    if (v.begin() == v.end()) return;

    uint32_t cubemap_tex = 0;
    for (auto e : registry.view<components::CubeMap, components::Render_Technique_CubeMap>()) {
        cubemap_tex = registry.get<components::CubeMap>(e).texture_handle;
        break;
    }
    if (cubemap_tex == 0) return;

    reflectiveTechnique_.bind(view, proj, cam_pos, cubemap_tex);
    for (auto e : v) {
        if (registry.has<components::Disabled>(e)) continue;
        auto& r     = registry.get<components::Renderable>(e);
        auto& xform = registry.get<components::Transform>(e);
        if (r.mesh == 0) continue;
        math::Mat4 model = math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale);
        reflectiveTechnique_.draw(r.mesh, model);
    }
    reflectiveTechnique_.unbind();
}

void RenderSystem::renderParticlePass(entities::Registry& registry,
                                       const math::Mat4& view,
                                       const math::Mat4& proj) {
    auto v = registry.view<components::Transform, components::ParticleCloud,
                            components::SimulationDomain>();
    if (v.begin() == v.end()) return;

    particleTechnique_.bind();
    for (auto e : v) {
        if (registry.has<components::Disabled>(e)) continue;
        auto& pc = registry.get<components::ParticleCloud>(e);
        if (pc.particle_count == 0 || pc.positions.empty()) continue;
        auto& xform = registry.get<components::Transform>(e);
        graphics::render_techniques::ParticleDrawData data{
            pc.positions.data(), pc.particle_count,
            {{"u_model", math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale)},
             {"u_view", view}, {"u_proj", proj}}
        };
        particleTechnique_.draw(data);
    }
    particleTechnique_.unbind();
}

void RenderSystem::renderVolumePass(entities::Registry& registry,
                                     const math::Mat4& view,
                                     const math::Mat4& proj,
                                     const math::Vec3& cam_pos) {
    auto v = registry.view<components::Transform, components::VolumeField,
                            components::SimulationDomain>();
    if (v.begin() == v.end()) return;

    volumeTechnique_.bind();
    for (auto e : v) {
        if (registry.has<components::Disabled>(e)) continue;
        auto& xform  = registry.get<components::Transform>(e);
        auto& vol    = registry.get<components::VolumeField>(e);
        auto& domain = registry.get<components::SimulationDomain>(e);
        if (!vol.interop_ready || vol.texture_handle == 0) continue;

        ensureVolumeProxy(registry, e, domain.nx, domain.ny, domain.nz);

        auto& vr = registry.get<components::VolumeRenderable>(e);
        if (vr.mesh == 0) continue;

        math::Vec3 box_min, box_max;
        computeWorldBounds(xform, domain.nx, domain.ny, domain.nz, box_min, box_max);

        graphics::render_techniques::VolumeDrawData data{
            vol.texture_handle, vr.mesh, domain.nx, domain.ny, domain.nz,
            {{"u_model",   math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale)},
             {"u_view",    view},
             {"u_proj",    proj},
             {"u_cam_pos", cam_pos},
             {"u_box_min", box_min},
             {"u_box_max", box_max}}
        };
        volumeTechnique_.draw(data);
    }
    volumeTechnique_.unbind();
}

// ── main update ───────────────────────────────────────────────────────────

void RenderSystem::update(entities::Registry& registry, const core::Window& window, float /*dt*/) {
    // ── Camera ──
    const components::Transform* cam_xform = nullptr;
    const components::Camera*   cam       = nullptr;
    for (auto e : registry.view<components::Camera, components::Transform>()) {
        cam       = &registry.get<components::Camera>(e);
        cam_xform = &registry.get<components::Transform>(e);
        break;
    }
    if (!cam || !cam_xform)
        throw std::runtime_error("No camera in scene.");

    int w, h; float aspect;
    window.getDimensions(w, h, aspect);

    math::Vec3 forward = (cam_xform->rotation * math::Vec3{0,0,-1}).norm();
    math::Vec3 up      = (cam_xform->rotation * math::Vec3{0,1, 0}).norm();
    math::Mat4 view    = math::Mat4::lookAt(cam_xform->position, cam_xform->position + forward, up);
    math::Mat4 proj    = math::Mat4::perspective(cam->fov_y_radians, aspect, cam->near_plane, cam->far_plane);

    // ── Passes ──
    renderCubemapPass(registry, view, proj);
    renderOpaquePass(registry, view, proj);
    renderReflectivePass(registry, view, proj, cam_xform->position);
    renderParticlePass(registry, view, proj);
    renderVolumePass(registry, view, proj, cam_xform->position);
}

} // namespace systems
} // namespace exd
