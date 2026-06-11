#pragma once
#include <SDL3/SDL.h>
#include <array>

namespace exd {
namespace core {

class EventState {
    public:
        EventState();
        EventState( SDL_Event* events, int num_events,  const bool* keyboardState,float mouseRelX, float mouseRelY); 
        ~EventState();
        void SetState( SDL_Event* events, int num_events, const bool* keyboardState, float mouseRelX, float mouseRelY);

        /// True only on the frame that `sc` was released.
        bool wasKeyReleased(SDL_Scancode sc) const {
            return sc < SDL_SCANCODE_COUNT && key_up_[sc];
        }

       const bool* keyboardState_;
       SDL_Event* events_;  
       int num_events_ = 0;
       float mouseRelX_ = 0;
       float mouseRelY_ = 0;

    private:
        friend class Window;
        std::array<bool, SDL_SCANCODE_COUNT> key_up_ = {};
};

} // namespace core
} // namespace exd
