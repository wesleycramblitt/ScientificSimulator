#include "core/event_state.hpp"
EventState::EventState() {}
EventState::EventState( SDL_Event* events, const int* num_events,  const bool* keyState)
    : keyboardState_(keyState), num_events_(num_events), events_(events){}
void EventState::SetState(SDL_Event* events,const int* num_events,  const bool* keyState) {
    events_ = events;
    num_events_ = num_events;
    keyboardState_ = keyState;
}

EventState::~EventState() { }
