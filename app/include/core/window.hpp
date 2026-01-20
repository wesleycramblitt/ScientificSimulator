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
        void HandleEvents();
        bool should_close = false;
       SDL_Window* window;
       SDL_GLContext context;
       EventState event_state;
};
