#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"

class FluidX3DSystem {

    public:
        FluidX3DSystem();
        void update(Registry& registry,Window& window, float dt);
        void start(Registry& registry, Window& window);

};
