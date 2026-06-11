#include "glad/gl.h"
#include "systems/polygon_mode_system.hpp"

namespace exd {
namespace systems {

void PolygonModeSystem::update(entities::Registry& registry, core::Window& window, float dt) {
    
    if (window.event_state.wasKeyReleased(SDL_SCANCODE_X)) {
        if (window.wireframe == false) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_CULL_FACE);
            window.wireframe = true;
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
            window.wireframe = false;
        }
    }
  

}

} // namespace systems
} // namespace exd
