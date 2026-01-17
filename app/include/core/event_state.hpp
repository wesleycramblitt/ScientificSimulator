#pragma once
#include <SDL3/SDL.h>

class EventState {
    public:
    EventState();
    EventState( SDL_Event* events,  const bool* keyboardState); 
    ~EventState();
    void SetState( SDL_Event* events, const bool* keyboardState);

    private:
       const bool* keyboardState_;
        SDL_Event* events_;  

};
