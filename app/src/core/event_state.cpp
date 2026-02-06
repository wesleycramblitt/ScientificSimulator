#include "core/event_state.hpp"
EventState::EventState() {}
EventState::EventState( SDL_Event* events, const int* num_events,  const bool* keyState,float mouseRelX, float mouseRelY)
    : keyboardState_(keyState), num_events_(num_events), events_(events), mouseRelX_(mouseRelX), mouseRelY_(mouseRelY){}
void EventState::SetState(SDL_Event* events,const int* num_events,  const bool* keyState,  float mouseRelX,  float mouseRelY) {
    events_ = events;
    num_events_ = num_events;
    keyboardState_ = keyState;
    mouseRelX_ = mouseRelX;
    mouseRelY_ = mouseRelY;
}

EventState::~EventState() { }
