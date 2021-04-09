#pragma once 

#include "../core/span.hpp"

#include <cstdint>

namespace rb {
    enum class builtin_shader : std::uint8_t {
        forward_vert,
        forward_frag,
        irradiance_vert,
        irradiance_frag,
        brdf_vert,
        brdf_frag,
        prefilter_vert,
        prefilter_frag,
        skybox_vert,
        skybox_frag
    };

    struct builtin_shaders {
        static span<const std::uint8_t> get(builtin_shader shader);
    };
}
