#pragma once 

#include <cstdint>

namespace rb {
    enum class vertex_attribute : std::uint8_t {
        position,
        texcoord,
        normal,
        blend_weight,
        blend_indices
    };
}
