#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/graphics_context.hpp"
#include "math/vec3.hpp"

namespace exd {
namespace components {
struct Transform;
} // namespace components

namespace systems {

class VolumeRenderSystem {
public:
    explicit VolumeRenderSystem(graphics::GraphicsContext& graphicsContext);

    void update(entities::Registry& registry, const core::Window& window, float dt);

private:
    void createProxyCube(entities::Registry& registry, entities::Entity e);
    void computeWorldBounds(const components::Transform& xform, int nx, int ny, int nz,
                            math::Vec3& out_min, math::Vec3& out_max);

    graphics::GraphicsContext& graphicsContext_;
};

} // namespace systems
} // namespace exd
