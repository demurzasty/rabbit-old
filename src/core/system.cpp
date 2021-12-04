#include <rabbit/core/system.hpp>

using namespace rb;

system_stage system::stage() const noexcept {
    return system_stage::update;
}

std::uint32_t system::execution_flags() const noexcept {
    return system_execution_flags::runtime_bit;
}

int system::priority() const noexcept {
    return 0;
}
