#include <rabbit/core/time.hpp>
#include <rabbit/core/settings.hpp>

using namespace rb;

float time::_elapsed_time{ 0.0f };
float time::_fixed_time{ 1.0f / 60.0f };

void time::setup() {
    _fixed_time = settings::time.fixed_time_step;
}

void time::release() {
}

float time::elapsed_time() {
    return _elapsed_time;
}

float time::fixed_time() {
    return _fixed_time;
}
