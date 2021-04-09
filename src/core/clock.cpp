#include <rabbit/core/clock.hpp>

#include <chrono>

using namespace rb;

long double clock::reset() {
    using duration = std::chrono::duration<long double>;

    const auto current = using_clock::now();
    const auto elapsed = std::chrono::duration_cast<duration>(current - _time);
    _time = current;

    return elapsed.count();
}
