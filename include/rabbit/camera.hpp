#pragma once 

namespace rb {
    enum class camera_type {
        perspective,
        orthogonal
    };

    struct camera {
        camera_type type{ camera_type::perspective };
        float field_of_view{ 45.0f };
        float size{ 11.25f };
    };
}
