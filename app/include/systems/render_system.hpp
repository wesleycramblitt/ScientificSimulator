#pragma once
#include "entities/registry.hpp"

class RenderSystem {
    public:
        RenderSystem();
        ~RenderSystem();
        void Update(Registry& registry, float dt);
};
