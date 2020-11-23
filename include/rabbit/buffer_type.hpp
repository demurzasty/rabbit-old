#pragma once 

#include <cstdint>

namespace rb {
    enum class buffer_type : std::int8_t {
        unknown = -1,

        vertex,
        index
    };
}