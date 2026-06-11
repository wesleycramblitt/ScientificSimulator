#pragma once
#include <SDL3/SDL.h>
#include <core/event_state.hpp>
#include <common/input_mode.hpp>
#include <vector>

namespace exd {
namespace core {

class Window {
    public:
        Window();
        ~Window();
        void die(const char * msg);
        void swapBuffers();
        void getDimensions(int& width, int& height, float& aspect) const;
        void getEvents();

        common::InputMode getInputMode() const { return input_mode_; }
        void setInputMode(common::InputMode mode);

        bool should_close = false;
        bool wireframe = false;
        bool grid_visible = true;
        bool simulation_mode = false;
        SDL_Window* window;
        SDL_GLContext context;
        EventState event_state;
        std::vector<SDL_Event> event_buffer;

    private:
        common::InputMode input_mode_ = common::InputMode::FPS;
};

} // namespace core
} // namespace exd
