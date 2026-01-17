#pragma once
#include "core/window.hpp"

class App {
    public:
        App();
        ~App();

        void Run();
    private:
        bool isRunning_;
        Window window_;
};

