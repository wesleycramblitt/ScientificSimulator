#pragma once
#include "entities/registry.hpp"
#include "core/event_state.hpp"

class RenderSystem {
    public:
        RenderSystem();
        ~RenderSystem();
        void Update(Registry& registry, EventState& eventState, float dt);
};
