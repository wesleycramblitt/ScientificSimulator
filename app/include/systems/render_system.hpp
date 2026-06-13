#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/render_techniques/cubemap_render_technique.h"
#include "graphics/render_techniques/lambertian_technique.hpp"
#include "graphics/render_techniques/reflective_technique.hpp"
#include "graphics/render_techniques/particle_render_technique.hpp"
#include "graphics/render_techniques/volume_render_technique.hpp"

namespace exd {
namespace systems {

class RenderSystem {
public:
    RenderSystem(graphics::GraphicsContext& graphicsContext);
    ~RenderSystem();

    void update(entities::Registry& registry, const core::Window& window, float dt);

private:
    void renderCubemapPass(entities::Registry& registry,
                           const math::Mat4& view, const math::Mat4& proj);
    void renderOpaquePass(entities::Registry& registry,
                          const math::Mat4& view, const math::Mat4& proj);
    void renderReflectivePass(entities::Registry& registry,
                              const math::Mat4& view, const math::Mat4& proj,
                              const math::Vec3& cam_pos);
    void renderParticlePass(entities::Registry& registry,
                            const math::Mat4& view, const math::Mat4& proj);
    void renderVolumePass(entities::Registry& registry,
                          const math::Mat4& view, const math::Mat4& proj,
                          const math::Vec3& cam_pos);
    void ensureVolumeProxy(entities::Registry& registry, entities::Entity e,
                           int nx, int ny, int nz);

    graphics::GraphicsContext& ctx_;

    graphics::render_techniques::CubeMapRenderTechnique  cubemapTechnique_;
    graphics::render_techniques::LambertianTechnique     lambertianTechnique_;
    graphics::render_techniques::ReflectiveTechnique     reflectiveTechnique_;
    graphics::render_techniques::ParticleRenderTechnique particleTechnique_;
    graphics::render_techniques::VolumeRenderTechnique   volumeTechnique_;
};

} // namespace systems
} // namespace exd
