#pragma once
#include <SDL3/SDL.h>
#include <core/event_state.hpp>

class Window {
    public:
        Window();
        ~Window();
        void die(const char * msg);
        void swapBuffers();
        void getDimensions(int& width, int& height, float& aspect) const;
        void getEvents();
        void handleEvents();
        bool should_close = false;
       SDL_Window* window;
       SDL_GLContext context;
       EventState event_state;
};
