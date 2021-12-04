#include <rabbit/core/clock.hpp>

using namespace rb;

float clock::reset() {
    const auto current_time = internal_clock::now();
    const auto elapsed_time = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - _time).count();
    _time = current_time;

    return elapsed_time;
}
