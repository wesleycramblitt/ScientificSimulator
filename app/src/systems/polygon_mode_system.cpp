#include "glad/gl.h"
#include "systems/polygon_mode_system.hpp"



void PolygonModeSystem::update(Registry& registry, Window& window, float dt) {
    
    if (window.event_state.keyboardState_[SDL_SCANCODE_X]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        window.wireframe = true;
    }
    else if (window.event_state.keyboardState_[SDL_SCANCODE_Z]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
        window.wireframe = false;
    }
  

}
