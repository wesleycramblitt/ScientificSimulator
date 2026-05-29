#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/mesh_manager.hpp"
#include "graphics/shader_manager.hpp"
#include "math/vec3.hpp"

struct Transform;

class VolumeRenderSystem {
public:
    explicit VolumeRenderSystem(MeshManager* meshManager);

    void update(Registry& registry, const Window& window, float dt);

private:
    void createProxyCube(Registry& registry, Entity e);
    void computeWorldBounds(const Transform& xform, int nx, int ny, int nz,
                            Vec3& out_min, Vec3& out_max);

    MeshManager*  meshManager_;
    ShaderManager shaderManager_;
};
