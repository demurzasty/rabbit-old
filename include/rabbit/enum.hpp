#pragma once 

#include <cstdint>
#include <type_traits>

namespace rb {
    template<typename T>
    constexpr std::size_t enum_size(T value) {
        return static_cast<std::size_t>(value);
    }
}
