#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"

namespace exd {
namespace systems {

class PolygonModeSystem {

    public:
        void update(entities::Registry& registry, core::Window& window, float dt);

};

} // namespace systems
} // namespace exd
