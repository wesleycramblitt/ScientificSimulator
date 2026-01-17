#include <ostream>
#include <iostream>
#include "core/window.hpp"
#include <glad/gl.h>
#include "core/event_state.hpp"

Window::Window() { 
    // initialize sDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        // Handle error
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // Create window
    window_ = SDL_CreateWindow(
        "Scientific Simulator",
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_EVENT_WINDOW_SHOWN
    );
    
    if (!window_) {
        // Handle error
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Create OpenGL context
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) {
        // Handle error
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Initialize OpenGL loader (like glad or glad2)
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        // Handle error
        std::cerr << "Failed to initialize opengl loader!" << std::endl;
        SDL_GL_DestroyContext(context_);
        SDL_DestroyWindow(window_);
        SDL_Quit();
        return;
    }
    
    // Set up OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

}
// Window::HandleEvents() {
    //  if (e.type == SDL_EVENT_QUIT) running = false;
    //   if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
    //
    //   if (e.type == SDL_EVENT_MOUSE_MOTION) {
    //     //SDL_Log("mouse rel: %f %f", (double)e.motion.xrel, (double)e.motion.yrel);
    //     float dx = (float)e.motion.xrel;
    //     float dy = (float)e.motion.yrel;
    //     yaw   += dx * mousesens;
    //     pitch -= dy * mousesens;
    //     if (pitch > 89.f) pitch = 89.f;
    //     if (pitch < -89.f) pitch = -89.f;
    //   }
    //
    //   if (e.type == SDL_EVENT_WINDOW_RESIZED) {
    //     glViewport(0, 0, e.window.data1, e.window.data2);
    //   }
    // }
    //
// }

void Window::GetEvents() {
    SDL_Event events[100]; //max of 100 events per frame
    int num_events = SDL_PeepEvents(events, 100, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
    
    const bool* keys = SDL_GetKeyboardState(nullptr);

    event_state_.SetState(events, keys); 
}

Window::~Window() {
  SDL_GL_DestroyContext(context_);
  SDL_DestroyWindow(window_);
  SDL_Quit();

}

void Window::Die(const char* msg) {
  const char* err = SDL_GetError();
  SDL_Log("FATAL: %s | SDL error: %s", msg, (err && *err) ? err : "(none)");
  std::exit(1);
}

void Window::SwapBuffers() {
    SDL_GL_SwapWindow(window_);
} 

void Window::GetDimensions(int& _width, int& _height, int& _aspect) {
    SDL_GetWindowSize(window_, &_width, &_height);
    _aspect = (_height > 0) ? (float)_width / (float)_height : 1.0f;
}
