#include <iostream>
#include "common/macros.hpp"
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
    window = SDL_CreateWindow(
        "Scientific Simulator",
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        // Handle error
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Create OpenGL context
    context = SDL_GL_CreateContext(window);
    if (!context) {
        // Handle error
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_GL_MakeCurrent(window, context);
    
    // Initialize OpenGL loader (like glad or glad2)
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        // Handle error
        std::cerr << "Failed to initialize opengl loader!" << std::endl;
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    
    // Set up OpenGL state
    GL_CALL(glEnable(GL_DEPTH_TEST));
    GL_CALL(glEnable(GL_CULL_FACE));
    GL_CALL(glCullFace(GL_BACK));
    GL_CALL(glViewport(0,0, 1280, 720));

    SDL_SetWindowRelativeMouseMode(window, true);
}


void Window::getEvents() {
    SDL_PumpEvents();

    SDL_Event events[100]; //max of 100 events per frame
    int num_events = SDL_PeepEvents(events, 100, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
    const bool* keys = SDL_GetKeyboardState(nullptr);

    float mouseRelX = event_state.mouseRelX_;
    float mouseRelY = event_state.mouseRelY_;

    for (size_t i{}; i < num_events; i++) {
         SDL_Event e = events[i];
          if (e.type == SDL_EVENT_QUIT) should_close = true;
          if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE) should_close = true;
          if (e.type == SDL_EVENT_MOUSE_MOTION) {
            mouseRelX += (float)e.motion.xrel;
            mouseRelY += (float)e.motion.yrel;
          }
          if (e.type == SDL_EVENT_WINDOW_RESIZED) {
            glViewport(0, 0, e.window.data1, e.window.data2);
          }
     }

    event_state.SetState(events, &num_events, keys, mouseRelX, mouseRelY); 
}

Window::~Window() {
  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

}

void Window::die(const char* msg) {
  const char* err = SDL_GetError();
  SDL_Log("FATAL: %s | SDL error: %s", msg, (err && *err) ? err : "(none)");
  std::exit(1);
}

void Window::swapBuffers() {
    SDL_GL_SwapWindow(window);
    glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);


} 

void Window::getDimensions(int& _width, int& _height, float& _aspect) const{
    SDL_GetWindowSize(window, &_width, &_height);
    _aspect = (_height > 0) ? (float)_width / (float)_height : 1.0f;
}
