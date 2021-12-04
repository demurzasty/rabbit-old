#pragma once 

#include <chrono>

namespace rb {
    class clock {
        using internal_clock = std::chrono::steady_clock;

    public:
        float reset();

    private:
        internal_clock::time_point _time{ internal_clock::now() };
    };
}
