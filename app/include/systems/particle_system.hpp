#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/graphics_context.hpp"

class LBM;

namespace exd {
namespace systems {

class ParticleSystem {
public:
    ParticleSystem(graphics::GraphicsContext& graphicsContext);
    void update(entities::Registry& registry, const core::Window& window, float dt, LBM* lbm);

private:
    void initGL(int particle_count);
    void uploadParticles(LBM* lbm, int& out_count);

    uint32_t gl_vao_ = 0;
    uint32_t gl_vbo_ = 0;
    int      gl_particle_count_ = 0;
    bool     gl_initialized_ = false;

    graphics::GraphicsContext& graphicsContext_;
};

} // namespace systems
} // namespace exd
