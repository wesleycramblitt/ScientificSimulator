#pragma once
#include <SDL3/SDL.h>
#include <core/event_state.hpp>

class Window {
    public:
        Window();
        ~Window();
        void Die(const char * msg);
        void SwapBuffers();
        void GetDimensions(int& width, int& height, int& aspect);
        void GetEvents();
    private:
       SDL_Window* window_;
       SDL_GLContext context_;
       EventState event_state_;
};
