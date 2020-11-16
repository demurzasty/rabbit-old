#pragma once 

#include <cstdint>

namespace rb {
    enum class image_format : std::int8_t {
        unknown = -1,

        r8,
        rg8,
        rgb8,
        rgba8,

        count
    };
}
