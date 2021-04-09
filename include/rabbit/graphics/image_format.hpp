#pragma once 

#include "../core/enum.hpp"

namespace rb {
    RB_ENUM(image_format, std::uint8_t, "r8", "rg8", "rgb8", "rgba8")
    enum class image_format : std::uint8_t {
        r8,
        rg8,
        rgb8,
        rgba8
    };
}
