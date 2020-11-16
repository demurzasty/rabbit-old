#pragma once 

#include <chrono>

namespace rb {
    class clock {
    public:
        using using_clock = std::chrono::steady_clock;

    public:
        long double reset();

    private:
        using_clock::time_point _time = using_clock::now();
    };
}