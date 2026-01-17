#include "core/event_state.hpp"
EventState::EventState() {}
EventState::EventState( SDL_Event* events,  const bool* keyState) : keyboardState_(keyState), events_(events){}
void EventState::SetState(SDL_Event* events,  const bool* keyState) {
    events_ = events;
    keyboardState_ = keyState;
}

EventState::~EventState() { }
