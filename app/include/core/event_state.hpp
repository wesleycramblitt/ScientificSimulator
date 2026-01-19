#pragma once
#include <SDL3/SDL.h>

class EventState {
    public:
        EventState();
        EventState( SDL_Event* events, const int* num_events,  const bool* keyboardState); 
        ~EventState();
        void SetState( SDL_Event* events,const int* num_events, const bool* keyboardState);

       const bool* keyboardState_;
       SDL_Event* events_;  
       const int* num_events_ = 0;

};
