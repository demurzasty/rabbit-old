#pragma once 

#include <cstdint>

namespace rb {
    enum class camera_type : std::uint8_t {
        perspective,
        orthogonal
    };

    struct camera {
        camera_type type{ camera_type::perspective };
        float fov{ 45.0f };
        float size{ 11.25f };
    };
}
