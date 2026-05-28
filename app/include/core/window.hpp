#pragma once
#include <SDL3/SDL.h>
#include <core/event_state.hpp>
#include <common/input_mode.hpp>
#include <vector>

class Window {
    public:
        Window();
        ~Window();
        void die(const char * msg);
        void swapBuffers();
        void getDimensions(int& width, int& height, float& aspect) const;
        void getEvents();

        InputMode getInputMode() const { return input_mode_; }
        void setInputMode(InputMode mode);

        bool should_close = false;
        bool wireframe = false;
        bool grid_visible = true;
        bool simulation_mode = false;
        SDL_Window* window;
        SDL_GLContext context;
        EventState event_state;
        std::vector<SDL_Event> event_buffer;

    private:
        InputMode input_mode_ = InputMode::FPS;
};
